// Persistence: saveData/VDGS json+txt export, FTP/SFTP upload, logging, version check,
// CTOT/taxi-zone URL loaders, multithread helper, xml lookup, de-ice id lookup.
// Moved verbatim out of CDMSingle.cpp. See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"
#include "third_party/pugixml/pugixml.hpp"
#include "src/net/SFTP.h"

using namespace pugi;

void CDM::saveData() {
    addLogLine("Called saveData...");
    try {
        bool found = false;
        vector<Plane> mySlotList = slotList;
        for (Plane plane : mySlotList) {
            found = false;
            bool foundData = false;
            string dataString = "";
            for (Plane planeSaved : slotListSaved) {
                if (plane.callsign == planeSaved.callsign && plane.isCdmAirport) {
                    found = true;
                    bool diffDataStored = false;
                    CFlightPlan fp = FlightPlanSelect(plane.callsign.c_str());
                    if (fp.IsValid()) {
                        dataString = getFlightStripInfo(fp, 0) + "/" + fp.GetFlightPlanData().GetDepartureRwy() + "/" +
                                     fp.GetFlightPlanData().GetSidName();
                    }
                    for (int z = 0; z < dataSaved.size(); z++) {
                        if (dataSaved[z].size() == 2) {
                            if (plane.callsign == dataSaved[z][0]) {
                                foundData = true;
                                if (dataString != dataSaved[z][1]) {
                                    dataSaved[z][1] = dataString;
                                    diffDataStored = true;
                                }
                                break;
                            }
                        }
                    }
                    if (plane.ttot != planeSaved.ttot || plane.tsat != planeSaved.tsat ||
                        plane.eobt != planeSaved.eobt || plane.flowReason != planeSaved.flowReason || diffDataStored) {
                        updateCdmDataApi(plane);
                    }
                    break;
                }
            }
            if (!found && plane.isCdmAirport) {
                // Plane is new in the slotList (not found int he latest slotListSaved)
                updateCdmDataApi(plane);
            }
            if (!foundData) {
                dataSaved.push_back({plane.callsign, dataString});
            }
        }

        // Set empty times as the plane is not anymore in the slotList
        for (Plane planeSaved : slotListSaved) {
            found = false;
            for (Plane plane : mySlotList) {
                if (plane.callsign == planeSaved.callsign) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                Plane myPlane(planeSaved.callsign, "", "", "", "", "", false, false, true);
                updateCdmDataApi(myPlane);
            }
        }
        slotListSaved = mySlotList;
        if (!ftpHost.empty()) {
            if (!mySlotList.empty()) {
                for (string airport : masterAirports) {
                    // Type2 -> https://fs.nool.ee/MSFS/VDGS/Specs/DATALINK.txt
                    if (vdgsFileType == "2" || vdgsFileType == "3") {
                        string fileName = dfad + "_" + airport + ".json";
                        createJsonVDGS(mySlotList, fileName, airport);
                    }
                    if (vdgsFileType == "1" || vdgsFileType == "3") {
                        ofstream myfile;
                        string fileName = dfad + "_" + airport + ".txt";
                        myfile.open(fileName, std::ofstream::out | std::ofstream::trunc);
                        for (Plane plane : mySlotList) {
                            if (myfile.is_open()) {
                                CFlightPlan fp = FlightPlanSelect(plane.callsign.c_str());
                                if (!fp.IsValid()) {
                                    continue;
                                }
                                if (airport == fp.GetFlightPlanData().GetOrigin()) {
                                    string str;
                                    if (plane.hasManualCtot && plane.ctot != "" && plane.ttot.length() >= 4) {
                                        str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot +
                                              "," + plane.ctot.substr(0, 4) + "," + plane.flowReason + ",";
                                    } else if (plane.hasManualCtot && plane.ttot.length() >= 4) {
                                        str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot +
                                              "," + plane.ttot.substr(0, 4) + ",MAN CTOT" + ",";
                                    } else {
                                        str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot +
                                              ",,flowRestriction" + ",";
                                    }
                                    myfile << str << endl;
                                }
                            }
                        }
                        myfile.close();
                        upload(fileName, airport, ".txt");
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception saveData " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception saveData");
    }
}

int CDM::getPlanePosition(string callsign) {
    try {
        for (int i = 0; i < slotList.size(); i++) {
            if (slotList[i].callsign == callsign) {
                return i;
            }
        }
        return -1;
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception getPlanePosition: " + (string)e.what());
        return -1;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception getPlanePosition");
        return -1;
    }
}

void CDM::createJsonVDGS(vector<Plane> slotList, string fileName, string airport) {
    addLogLine("Called createJsonVDGS...");
    try {
        Document document;
        document.SetObject();
        Value version;
        version.SetInt(1);
        document.AddMember("version", version, document.GetAllocator());

        Value flightsArray(kArrayType);

        for (Plane plane : slotList) {
            CFlightPlan fp = FlightPlanSelect(plane.callsign.c_str());
            if (!fp.IsValid()) {
                continue;
            }
            if (fp.GetFlightPlanData().GetOrigin() == airport) {
                // Check if RadarTarget is valid before accessing it
                CRadarTarget rt = RadarTargetSelect(plane.callsign.c_str());
                if (!rt.IsValid()) {
                    continue;
                }
                
                // Check if the correlated flight plan is valid
                if (!rt.GetCorrelatedFlightPlan().IsValid()) {
                    continue;
                }
                
                string tobtString = "", tsatString = "";
                if (plane.eobt.length() >= 4) {
                    tobtString = plane.eobt;
                }
                if (plane.tsat.length() >= 4) {
                    tsatString = plane.tsat;
                }
                Value flight(kObjectType);
                Value lat;
                lat.SetDouble(rt.GetPosition().GetPosition().m_Latitude);
                Value lon;
                lon.SetDouble(rt.GetPosition().GetPosition().m_Longitude);
                Value icao_type(rt.GetCorrelatedFlightPlan()
                                    .GetFlightPlanData()
                                    .GetAircraftFPType(),
                                document.GetAllocator());
                Value callsign(plane.callsign.c_str(), document.GetAllocator());
                Value destination(rt.GetCorrelatedFlightPlan()
                                      .GetFlightPlanData()
                                      .GetDestination(),
                                  document.GetAllocator());
                Value tobt(tobtString.substr(0, 4).c_str(), document.GetAllocator());
                Value tsat(tsatString.substr(0, 4).c_str(), document.GetAllocator());

                flight.AddMember("lat", lat, document.GetAllocator());
                flight.AddMember("lon", lon, document.GetAllocator());
                flight.AddMember("icao_type", icao_type, document.GetAllocator());
                flight.AddMember("callsign", callsign, document.GetAllocator());
                flight.AddMember("destination", destination, document.GetAllocator());
                flight.AddMember("tobt", tobt, document.GetAllocator());
                flight.AddMember("tsat", tsat, document.GetAllocator());
                string slot = "";
                if (plane.hasManualCtot && plane.ttot.length() >= 4) {
                    Value ctot(plane.ttot.substr(0, 4).c_str(), document.GetAllocator());
                    flight.AddMember("ctot", ctot, document.GetAllocator());
                }
                Value runway(rt.GetCorrelatedFlightPlan()
                                 .GetFlightPlanData()
                                 .GetDepartureRwy(),
                             document.GetAllocator());
                Value sid(rt.GetCorrelatedFlightPlan()
                              .GetFlightPlanData()
                              .GetSidName(),
                          document.GetAllocator());
                flight.AddMember("runway", runway, document.GetAllocator());
                flight.AddMember("sid", sid, document.GetAllocator());
                flightsArray.PushBack(flight, document.GetAllocator());
            }
        }

        document.AddMember("flights", flightsArray, document.GetAllocator());

        // Convert the document to a JSON string
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        document.Accept(writer);

        ofstream outFile;
        outFile.open(fileName, std::ofstream::out | std::ofstream::trunc);
        if (outFile.is_open()) {
            outFile << buffer.GetString() << std::endl;
            outFile.close();
        }

        upload(fileName, airport, ".json");
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception createJsonVDGS: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception createJsonVDGS");
    }
}

bool CDM::isNumber(string s) { return std::any_of(s.begin(), s.end(), ::isdigit); }

void CDM::upload(string fileName, string airport, string type) {
    if (sftpConnection) {
        uploadSftp(fileName, airport, type);
    } else {
        uploadFtp(fileName, airport, type);
    }
}

void CDM::uploadSftp(string fileName, string airport, string type) {
    addLogLine("Called uploadSftp...");
    string saveName = "/CDM_data_" + airport + type;
    int response = UploadFileFTPS(ftpHost, ftpUser, ftpPassword, fileName, saveName);
    if (response != 0) {
        sendMessage("FTP error: " + response);
    }
}

void CDM::uploadFtp(string fileName, string airport, string type) {
    addLogLine("Called uploadFtp...");
    try {
        string saveName = "/CDM_data_" + airport + type;
        HINTERNET hInternet = InternetOpen("CDM", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            addLogLine("ERROR: InternetOpen failed: " + std::to_string(GetLastError()));
            return;
        }
        HINTERNET hFtpSession = InternetConnect(hInternet, ftpHost.c_str(), INTERNET_DEFAULT_FTP_PORT, ftpUser.c_str(),
                                                ftpPassword.c_str(), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
        if (!hFtpSession) {
            addLogLine("ERROR: InternetConnect failed: " + std::to_string(GetLastError()));
            InternetCloseHandle(hInternet);
            return;
        }
        BOOL ftpResult = FtpPutFile(hFtpSession, fileName.c_str(), saveName.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
        if (!ftpResult) {
            addLogLine("ERROR: FtpPutFile failed: " + std::to_string(GetLastError()));
        }
        InternetCloseHandle(hFtpSession);
        InternetCloseHandle(hInternet);
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception upload: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception upload");
    }
}

void CDM::addLogLine(string text) {
    text = GetTimeNow() + ": " + text;
    std::ofstream file(tfad, std::ios::app);

    if (file.is_open()) {
        file << text << std::endl;
    }
    file.close();
}

void CDM::removeLog() {
    // Remove the file
    remove(tfad.c_str());
}

int CDM::GetVersion() {
    const std::string url = "https://raw.githubusercontent.com/rpuig2001/CDM/master/version.txt";

    const auto response = restclient_->get(url);

    // Check if it is not a beta version
    // if (std::string(MY_PLUGIN_VERSION).find("b") == std::string::npos) {
    //     // Check version
    //     if (!response.body.empty() && response.body.find(MY_PLUGIN_VERSION) == std::string::npos) {
    //         const std::string display_msg = "Please UPDATE YOUR CDM PLUGIN, version " + response.body +
    //                                         " is OUT! You have version " +
    //                                         MY_PLUGIN_VERSION " installed, download it from vats.im/CDM";
    //         DisplayUserMessage(MY_PLUGIN_NAME, "UPDATE", display_msg.c_str(), true, false, false, false, false);
    //     }
    // }

    return -1;
}

bool CDM::getCtotsFromUrl(string code) {
    addLogLine("Called getCtotsFromUrl...");
    try {
        evCtots.clear();
        slotFile.clear();
        string vatcanUrl = code;
        const auto response = restclient_->get(vatcanUrl);
        int responseCode = response.statusCode;

        if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
            // handle error 404
            sendMessage("UNABLE TO LOAD SLOTs...");
        } else {
            std::istringstream is(response.body);

            // Get data from .txt file
            string lineValue;
            int i = 0;
            while (getline(is, lineValue)) {
                addVatcanCtotToEvCTOT(lineValue);
                i++;
            }
            if (i <= 1) {
                ctotCid = false;
                addLogLine("No Ctots in file, disabling EvCTOT...");
            }
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception getCtotsFromUrl: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception getCtotsFromUrl");
    }

    return true;
}

bool CDM::getTaxiZonesFromUrl(string url) {
    try {
        const auto response = restclient_->get(url);
        int responseCode = response.statusCode;

        if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
            // handle error 404
            sendMessage("UNABLE TO LOAD TaxiZones URL...");
        } else {
            std::istringstream is(response.body);

            // Get data from .txt file
            string lineValue;
            while (getline(is, lineValue)) {
                if (!lineValue.empty()) {
                    if (lineValue.substr(0, 1) != "#") {
                        if (lineValue.length() > 1) {
                            if (isdigit(lineValue[lineValue.length() - 1])) {
                                TxtTimesVector.push_back(lineValue);
                            } else {
                                TxtTimesVector.push_back(lineValue.substr(0, lineValue.length() - 1));
                            }
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception getTaxiZonesFromUrl: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception getTaxiZonesFromUrl");
    }

    return true;
}

// Multithread Run Functions
void CDM::multithread(void (CDM::*f)()) {
    try {
        runDetachedTask(f);
    } catch (const std::exception&) {
        cout << "Failed to multi-thread function";
    }
}

void CDM::waitForDetachedTasks(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(asyncTasksMutex_);
    asyncTasksCv_.wait_for(lock, timeout, [this] { return activeAsyncTasks_.load(std::memory_order_acquire) == 0; });
}

// Get Data from the xml file
string CDM::getFromXml(string xpath) {
    xml_document doc;

    // load the XML file
    doc.load_file(pfad.c_str());

    pugi::xpath_node_set altPugi = doc.select_nodes(xpath.c_str());

    std::vector<std::string> result;
    for (auto xpath_node : altPugi) {
        if (xpath_node.attribute() != nullptr)
            result.push_back(xpath_node.attribute().value());
        else
            result.push_back(xpath_node.node().child_value());
    }

    if (result.size() > 0) {
        return result[0];
    } else {
        return "";
    }
}

int CDM::getDeIceId(string callsign) {
    for (vector<string> deice : deiceList) {
        if (deice[0] == callsign) {
            if (deice[2].find("REM") != std::string::npos && deice[2].length() == 4) {
                return stoi(deice[2].substr(3, 1));
            } else if (deice[1] == "STND") {
                return 0;
            }
        }
    }
    return -1;
}
