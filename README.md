# CDM plugin V2.2
CDM is an Euroscope plugin based on the real life CDM tool that allows us to improve the departure flows at airports.
If you want to support the development, you can do it in: https://ko-fi.com/rpuig

CDM includes the following times:
- EOBT: Estimated off block time.
- TOBT: Target off block time.
- TSAT: Target Start-Up Approval Time.
- TTOT: Target Take Off Time.
- TSAC: Target Start-Up Approval Communicated.
- ASAT: Actual Start-Up Approval Time.
- ASRT: Actual Start-Up Request Time.
- CTOT: Calculated Take Off Time.


## How to use:
- Load up the plugin.
- If there is no master controller, you should use the command ``.cdm master {airport}`` of the airport you want to become the master. You can have as many airports as you want, but there can ony be **1 MASTER at the same time** (The MASTER should be DELIVERY or the lowest ATC position to have access to all CDM actions).
- All items and actions available:

- EOBT

  ![image](https://user-images.githubusercontent.com/68125167/210136450-3b4b7fea-8f80-441a-ba3f-b95e3d8ca5d1.png)

    or

  ![image](https://i.gyazo.com/928f2e35f0a4248e17442bba552d72e0.png)

- E

  ![image](https://i.gyazo.com/436e8eb7b20b00d2c39a483319d03425.png)

- TOBT

  ![image](https://user-images.githubusercontent.com/68125167/210136458-86422bb9-d3cc-4ac1-a8a1-0c25d5ac9f7b.png)

    or

  ![image](https://i.gyazo.com/ad6344055e7de91ab8a386f7153d19e1.png)

- TSAT

  ![image](https://i.gyazo.com/37b0ad531fc4a32dfaffbf7db83c5546.png)

- TTOT

  ![image](https://i.gyazo.com/4533873ef8d8342cb5b35ed381bb0f47.png)

- TSAC

  ![image](https://user-images.githubusercontent.com/68125167/210136465-21a3d004-38a0-4261-8055-c22ca424d7b1.png)

    or

  ![image](https://i.gyazo.com/8f9d55ec477a8c21ddb63df3b4da15a1.png)

- ASAT

  ![image](https://i.gyazo.com/cf08823153ce9e99e1936659c07ad67d.png)

- ASRT

  ![image](https://user-images.githubusercontent.com/68125167/210136468-33b8384a-aa92-47dc-9512-ae7dbe8eaed0.png)

- CTOT

  ![image](https://user-images.githubusercontent.com/68125167/210136480-babb9ec2-6989-4302-91ac-a82de59ecadf.png)

    or

  ![image](https://i.gyazo.com/775e1bf69fac29e2e3a776d35e67952a.png)
 
- Network Status
  
  ![image](https://github.com/user-attachments/assets/0839e1d7-9f5a-4d95-98d3-950cb1cd299d)

- De-Ice

  ![image](https://github.com/user-attachments/assets/f557481f-5c16-4c12-a09e-871f818bc656)


- Ready Start-up

  ![image](https://i.gyazo.com/842144f7bddf11f3c9165c42ef0f940e.png)

- Extra (All CDM Option in one Menu)

  ![image](https://user-images.githubusercontent.com/68125167/210136505-9b46b673-537d-4e2c-86aa-be36add431dd.png)



## MASTER AND SLAVE:
- Master: The master is the "admin" of the CDM and is the only controller who calculates the CDM times.
  - Use ``.cdm master {airport}`` command (**ONLY 1 CONTROLLER CAN BE THE MASTER AT THE SAME TIME, in case of an alreadz existing master. CDM will not be able to set another master**).
- Slave: The Slave Monitors the CDM and has some limited actions.
  - Default type, so, you don't need to change anything unless you are now a master, where you can use ``.cdm slave {airport}`` command.

### HOW TO DO A CONTROLLER CHANGE CORRECTLY:
1. The **Old controller** changes to Slave with command ``.cdm slave``.
3. Once there are no master controllers, the **new controlles** gets the master "rol" with the command ``.cdm master {airport}``.
4. That's it!

## Define configurations

### Colors.txt
  - color1 = DARK GREEN
  - color2 = LIGHT GREEN
  - color3 = GREY
  - color4 = ORANGE
  - color5 = YELLOW
  - color6 = DARKYELLOW
  - color7 = RED
  - color8 = EOBT STATIC COLOR
  - color9 = TTOT STATIC COLOR
  - color10 = ASRT STATIC COLOR
  - color11 = CTOT STATIC COLOR
  - color12 = CHANGES TOBT, TSAT and ASAT to the defined color WHEN S/U STATUS IS SET
 
### CDMconfig.xml
  - Normal Visibility Operations Rate/hour (ex. rate ops="40").
  - Low Visibility Operations Rate/hour (ex. rateLvo ops="10").
  - Expired CTOT time, it selects the time before expire the CTOT if the pilot is not connected (ex. expiredCtot time="15").
  - Real Mode to calculate times automatically from the sent EOBT (**DISABLED:** realMode mode="false" and **ENABLED:** realMode mode="true").
  - Pilot Tobt to enable/disable the automatic update of TOBT by Pilots (**DISABLED:** pilotTobt mode="false" and **ENABLED:** pilotTobt mode="true").
  - Invalidate flight at tsat will invalidate flights at TSAT+6 (ex. invalidateAtTsat mode=true).
  - [OPTIONAL] Rates URL (ex. Rates url="https://........"), if no URL needed, just leave it blank (ex. Rates url="") and the file will be used.
  - [OPTIONAL] Taxizones URL (ex. Taxizones url="https://........"), if no URL needed, just leave it blank (ex. Taxizones url="") and the file will be used.
  - [OPTIONAL] Event CTOTs URL to the TXT file - Format is defined below (ex. Ctot url:"https://...."), if no URL needed, just leave it blank (ex. Ctot url="").
  - [OPTIONAL] sidInterval URL to the TXT file - Format is defined below (ex. sidInterval url:"https://...."), if no URL needed, just leave it blank to disable the sidInterval functionallity (ex. sidInterval url="").
  - Default Taxi time in minutes if taxi time not found in the taxizones.txt file (ex. DefaultTaxiTime minutes="15").
  - [OPTIONAL] DeIceTimes by Wtc definition (ex. "<DeIceTimes light="5" medium="9" heavy="12" super="15"/>") If no defined, values 5, 9, 12 and 15 are used internally.
  - Refresh Time in seconds (ex. RefreshTime seconds="20").
  - Debug mode activated (true) or desactivated (false) (ex. Debug mode="false" or Debug mode="true").
  - [OPTIONAL] In case of ECFMP use, the api url needs to be set (Default -> FlowRestrictions url:"https://ecfmp.vatsim.net/api/v1/plugin"), if no URL needed, just leave it blank (ex. FlowRestrictions url="").
  - VDGS file type: 0-None, 1-TXT, 2-JSON, 3-TXT&JSON (ex. vdgsFileType type="3").
  - [OPTIONAL] FTP host to push CDM Data (ex. ftpHost host:"ftp.aaaaaa.com") - leave it blank to use CDM server.
  - [OPTIONAL] FTP user to push CDM Data (ex. ftpUser user:"username") - leave it blank to use CDM server.
  - [OPTIONAL] FTP password to push CDM Data (ex. ftpPassword password:"&&&&&&") - leave it blank to use CDM server.
  - Server Communication, enabled/disables all server features (**DISABLED:** Server mode="false" and **ENABLED:** Server mode="true").
  - SU_WAIT sets a remark in FlightStrip for external use when ready TOBT is pressed (ex. Su_Wait mode="false" or Su_Wait mode="true").
 
### taxizones.txt
  - You can define a zone with an specific taxiTime with the following specifications ``AIRPORT:RUNWAY:BOTTOM_LEFT_LAT:BOTTOM_LEFT_LON:TOP_LEFT_LAT:TOP_LEFT_LON:TOP_RIGHT_LAT:TOP_RIGHT_LON:BOTTOM_RIGHT_LAT:BOTTOM_RIGHT_LON:TAXITIME``, ex:``LEBL:25L:41.286876:2.067318:41.290236:2.065955:41.295688:2.082523:41.292662:2.084613:10``, if no taxizone defined, the default taxi time is set to 15 min.

### rate.txt

  Format:
  `AIRPORT:A:ArrRwyList:NotArrRwyList:D:DepRwyList:NotDepRwyList:DependentRwyList:Rate_RateLvo`

    - ArrRwyList -> Comma-separated list of runways (If more than 1, it will use if one, other or all selected). Enter * to disregard.

    - NotArrRwyList -> Comma-separated list of runways. Enter * to disregard.

    - DepRwyList -> Comma-separated list of runways (If more than 1, it will use if one, other or all selected). Enter * to disregard.

    - NotDepRwyList -> Comma-separated list of runways. Enter * to disregard.

    - DependentRwyList -> Comma-separated list of runways (it will use the same sequence order for runways selected here). Enter * to disregard.

    - Rate_RateLvo -> Normal Rate and LVO Rate. If more than one departure runways, you can define more than one rate separated by comma.


  Examples:

    - `LEPA:A:24L:*:D:24R,24L:*:24L,24R:30_12` (1 arr runway, 1 dep runway, 24R/L as dependant. 1 rate defined for all departures).

    - `LEPA:A:24L:24R:D:24R,24L:*:*:30_12,20_7` (1 arr runway, 1 non-arrival runway, 2 dep runway, dep runways as independant, different rates defined for both dep runways).

    - `LEPA:A:*:*:D:*:*:*:30_12` (All departures would have the same rate, doesn't matter the selected runways).

  Internal Checks:
  
  A line will be "activated" based on:

   - Runway assigned to the plane. (This runway should be in the `DepRwyList` list of the line. Which must comply with the below point).

   - Runways selected in Euroscope's Runway selector dialog (if the selected runways are in the line. If more runways in the line than selected in ES, but the selected are as `DepRwyList` in the line, it will be activated too).
   
<br>

  Important points
  - **Order of the configurations/rates is important (first line more important than last).**
  - **AIRPORTS NOT DEFINED, WILL NOT BE CONSIDERED A CDM AIRPORT.**
  - **Examples can be found in the givenfiles.**
  
  
  <br>

*Examples can be found in the givenfiles.*



## Event CTOTs
### How does it work?
Used for the EVCTOT column with the following format: <cid>,<CTOT>

Example:

```
9999999,0800
9999999,0802
9999999,0804
9999999,0806
9999999,0808
9999999,0820
9999999,0822
9999999,0824
9999999,0828
9999999,0836
9999999,0840
9999999,0842
9999999,0844
```

## Sid Interval
### How does it work?
Used to seperate planes based on departure SID Ponint. Format: <ICAO_Airport>,<dep_rwy>,<SID1>,<SID2>,<seperation_minutes>

Notes:
- SID are sid points. For sid LARPA4Q, "LARPA" is used as SID, and "4Q" should not be included.
- Seperation minutes are decimals, so a decimal or non-decimal value can be set. 1.0, 7 and 15.2 are possible values.

Example:

```
LEPA,24R,ESPOR,BAVER,10
LEPA,24R,BAVER,EPAMA,15.5
```

## FTP files and format
### Files
Every airport will have a different txt file (ex. LEBL airport: CDM_data_LEBL.txt)

### TXT Format
``CALLSIGN,TOBT,TSAT,TTOT,CTOT,FlowRestrictionMessage,``
```
BAW224,183600,183600,184400,ctot,flowRestriction,
RYR22GV,183600,183700,184600,ctot,flowRestriction,
MON562,183600,183700,184800,ctot,flowRestriction,
IBE73RT,183600,184000,185000,ctot,flowRestriction,
IBE51D,183600,184200,185200,ctot,flowRestriction,
IBE3540,183600,184400,185400,ctot,flowRestriction,
EXS15G,183600,184900,185600,ctot,flowRestriction,
EXS12,183600,184700,185800,1922,London Event,
MON837,183600,185200,190000,ctot,flowRestriction,
RYR33P,183600,185200,190200,ctot,flowRestriction,
MON235N,183600,185300,190400,ctot,flowRestriction,
EZY12JM,183600,185600,190600,ctot,flowRestriction,
RYR42TQ,183600,185700,190800,ctot,flowRestriction,
BEE154A,183600,190000,191000,1924,London Event,
```

### JSON Format
```
{
   "version":1,
   "flights":[
      {
         "lat":41.2923,
         "lon":2.09795,
         "icao_type":"A320",
         "callsign":"CFG7521",
         "flight":"CFG7521",
         "tobt":"1910",
         "tsat":"1910",
         "runway":"24L",
         "sid":"MOPAS5Q"
      },
      {
         "lat":41.2915,
         "lon":2.07371,
         "icao_type":"A320",
         "callsign":"BAW483B",
         "flight":"BAW483B",
         "tobt":"1913",
         "tsat":"1913",
         "runway":"24L",
         "sid":"MOPAS5Q"
      }
   ]
}
```

## CAD - Capacity Availability Document (Server-side)
### CAD - Arrival Airports (Server-side)
To define arrival airport capacities, this Document (https://raw.githubusercontent.com/rpuig2001/Capacity-Availability-Document-CDM/main/CAD.txt) can be filled up.
It takes all the Vatsim network flight paths and calculates an arrival time. In case of the arrival time being speprated less than the minimum defined, the plane will be delayed by a CTOT.
CTOT and FM will be showing in the CDM plugin if traffic is affected.

### CAD - Airspaces (Server-side)
To define arrival airspaces capacities, this Document (https://github.com/rpuig2001/Capacity-Availability-Document-CDM/blob/main/airspaces.geojson) can be filled up.
It takes all the Vatsim network flight paths and calculates a 4D path (longitudinal, vertical and entry/exit times) based on the filed flightplan. In case the capacity of an specified sector is greater than the defined, the plane will be delayed by a CTOT.
CTOT and FM will be showing in the CDM plugin if traffic is affected.

### CAD - More info
For more information, check the CAD GitHub Repository.
https://github.com/rpuig2001/Capacity-Availability-Document-CDM

### VDGS
Pilots can use the CDM vdgs - https://vats.im/vdgs
### What can pilots see?
From  there, pilots can monitor their EOBT, TOBT, TSAT, CTOT and SID.
### What can pilots modify?
The TOBT can be modified. It will have a direct effect to the plugin if _"PilotTobt"_ option is enabled.

![image](https://github.com/user-attachments/assets/409d5241-872f-4f94-ae8f-c454cd905c48)


## Commands
- ``.cdm refresh`` - Force the refresh phase to do it now.
- ``.cdm save`` - Saves data to savedData.txt.
- ``.cdm master {airport}`` - Become the master of the selected airport.
- ``.cdm slave {airport}`` - Turn back to slave of the selected airport.
- ``.cdm refreshtime {seconds}`` - It changes the refresh rate time in seconds (Default 30, MAX 99 Seconds).
- ``.cdm customdelay {airport}/{runway} {time_start}`` - Moves all TSATs for selected airport and runway from the starting at the time_start (time_start can be a 4 digits time (2114 - 21:14 time) or 1/2 digits minutes (5 - 5min or 10 - 10 min) - WAIT SOME SECONDS TO UPDATE AFTER APPLIED. (Ex1. ``.cdm customdelay LEBL/24L 1100`` -> All TSATs from LEBL rwy 24L will start at 1100 // Ex2. ``.cdm customdelay LEBL/24L 10`` -> All TSATs will start at now+10 min). To remove the "restriction" use -> ".cdm customdelay LEBL/24L 9999" (using 9999 as time).
- ``.cdm lvo`` - Toggle lvo ON or OFF.
- ``.cdm realmode`` - Toggle realmode ON or OFF.
- ``.cdm server`` - Toggle server communication ON or OFF.
- ``.cdm remarks`` - Toggle set TSAT to Euroscope scratchpad ON or OFF.
- ``.cdm rate`` - Updates rates values from rate.txt.
- ``.cdm flow`` - Reloads the flow data (Otherwise it's automatically reloaded every 5 min).
- ``.cdm help`` - Sends a message with the available commands.

## Functions and colors:

- Column EOBT: It gets the EOBT set by the pilot in the flightplan.
  - NOTES:
    - If **RealMode** is enabled, when the pilot send a new EOBT, then it will show with ``color4`` (Default ORANGE) when **EOBT is different than TOBT**.
  - Functions
    - ``Edit EOBT`` -> Sets EOBT to the specified time (4 digits).
  - Colors:
    - ``color8`` -> Default.

- Column TOBT: If realMode is disabled, TOBT will calculate TSAT and TTOT from the TOBT time. To delete it simple edit the time and press enter deleting the content.
  - NOTES:
    - If there is no ASRT or "Ready Start-up GREEN", at TOBT+5, TSAT and other times will be invalidated.
    - To add a TOBT while realMode is DISABLED, use the Ready TOBT Function to set the actual time as a TOBT or the Edit TOBT to set a 4 digits time.
    - If realMode is enable it will ONLY set the EOBT as TOBT when the first flightplan is recived, if the EOBT is changed the TOBT will not change automatically and you can use other functions such as the EOBT to TOBT Function to move it through. (EOBT will have a different color to say you that there's a new time sent by the pilot).
  - Functions:
    - ``Ready TOBT`` -> Sets TOBT to the actual time and sends a CDM-Network REA message (for CTOT improvement).
    - ``Edit TOBT`` -> Sets TOBT to the specified time (4 digits).
  - Colors:
    - ![#8fd894](https://img.shields.io/badge/-8fd894) `LIGHT GREEN` -> Before EOBT -5.
    - ![#00c000](https://img.shields.io/badge/-00c000) `DARK GREEN` -> After EOBT-5.
    - ![#f5ef0d](https://img.shields.io/badge/-f5ef0d) `YELLOW` -> Last minute of TOBT (with ASRT set or no).

- Column E: It shows a letter depending on the plane timmings:
  - Functions:
    - NO FUNCTIONS.
  - Colors:
    - ![#00c000](https://img.shields.io/badge/-00c000) `DARK GREEN` -> Default.
  - TEXT SHOWING:
    - P: EOBT is farther than the Actual Time - 35min. TSAT, TTOT and TOBT will be showing the following character "~~" (To order them to the end of the list).
    - C: EOBT is less than 35min and TOBT hasn't expired (TOBT+6) or TSAT hasn't expired (TSAT+6).
    - I: TSAT has expired.

- Column TSAT: It is the TTOT - the taxi time defined in the taxizones.txt, otherwise it sets 15min.
  - Functions:
    - NO FUNCTIONS.
  - Colors:
    - ![#8fd894](https://img.shields.io/badge/-8fd894) `LIGHT GREEN` -> From EOBT-35 to TSAT-5 and after TSAT+6 if not expired.
    - ![#00c000](https://img.shields.io/badge/-00c000) `DARK GREEN`  -> From TSAT-5 to TSAT+5.
    - ![#f5ef0d](https://img.shields.io/badge/-f5ef0d) `YELLOW` -> From TSAT+5 to TSAT+6.

- Column TTOT: The plugin calculates a TSAT based on this column, the TTOT, you can't have planes with same TTOT, the time between departures is calculated from the rate/hour. So if you need 40 departures/hour, the plugin will calculate it for you with no equal TTOTs.
  - Functions:
    - ``Edit/Set custom CDT`` -> Sets CDT as desired if CDT is available (for example, if a CDT is already used by another pilot TTOT, it would not be set as it's already in use).
  - Colors:
    - ``color9`` -> Default.

- Column TSAC: With the left click you can directly set the tsat and with the right click you can remove it or set the time you want. If this field is +/- 5min that the TSAT, the color change to orange to indicate that his TSAT has changed more than 5min.
  - Functions:
    - NO FUNCTIONS.
  - Colors:
    - ![#00c000](https://img.shields.io/badge/-00c000) `DARK GREEN` -> If between +/- 5min of TSAT.
    - ![#ed852e](https://img.shields.io/badge/-ed852e) `ORANGE` -> If +/- 5min of TSAT.

- Column ASAT: It sets the time when ST-UP, TAXI or DEPA state is set on the first time.
  - Functions:
    - NO FUNCTIONS.
  - Colors:
    - ![#00c000](https://img.shields.io/badge/-00c000) `DARK GREEN` -> If actual time < ASAT - 5min.
    - ![#f5ef0d](https://img.shields.io/badge/-f5ef0d) `YELLOW` -> From ASAT+5 to always.

- Column ASRT: It shows the requested StartUp time, It can be added to the list with the toggle function or sending a REA Msg.
  - Functions:
    - ``Toggle ASRT`` -> Sets RSTUP/ASRT or removes it if already set.
  - Colors:
    - ``color10`` -> Default.
  
- Column Ready Start-up: It shows if the plane is Ready for Start-up or not together with the ASRT. (ASRT and Ready Start-up do the same, but this column is a way to represent the real "ready start-up" state from IRL because Euroscope doesn't have this state).
  - Functions:
    - ``Toggle Ready Start-up function`` -> Sets RSTUP/ASRT or removes it if already set.
  - Colors:
    - ![#00c000](https://img.shields.io/badge/-00c000) ``GREEN`` -> RSTUP is set.
    - ![#BE0000](https://img.shields.io/badge/-BE0000) ``RED`` -> RSUP is NOT set.
   
- Column De-Ice: It modifies taxi time accordingly based on acft Wtc to adjust departure sequence with de-icing.
  - Options:
    - NONE: No extra EXOT minutes are added.
    - REM: For a REMOTE de-ice. De-ice minutes are added to EXOT and in case of 20 min of EXOT, there will be a diference of 20 min between TSAT and TTOT.
    - STND: For a STAND de-ice. De-ice minutes are added between TOBT and TSAT and between TSAT and TTOT, the normal taxi time applies.
  - Functions:
    - ``Toggle Ready Start-up function`` -> Sets NONE/REM/STND.
  - Colors:
    - ![#f5ef0d](https://img.shields.io/badge/-f5ef0d) `YELLOW` -> REM (REMOTE) or STAND de-ice selected.

- Column CTOT: It shows aircraft's CTOT which can be added, modified, removed or reloaded.
  - Functions:
    - ``Set/Remove MANUAL CTOT`` -> It creates or removes a CTOT for the user (Only CTOTs where TSAT>now+5min are allowed).
   
  - Prioritize:
    - In case of ECFMP CTOT, has priority over a CDM server restriction.
  - Colors:
    - ``color11`` -> Default.
    - ![#00c000](https://img.shields.io/badge/-00c000) `GREEN` -> CDM Server CTOT.
    - ![#ed852e](https://img.shields.io/badge/-ed852e) `ORANGE` -> MANUAL/EVENT CTOT.
    - ![#BE0000](https://img.shields.io/badge/-BE0000) `RED` -> Flow/CAD CTOT.
   
- Column Network Status: Shows the STS from the CDM-Network.
  - Status/Funcions:
    - ![#f5ef0d](https://img.shields.io/badge/-f5ef0d) ``REA`` -> Sends a REA message to find the best possible CTOT (Only shows in case CTOT exists).
    - ![#00c000](https://img.shields.io/badge/-00c000) ``PRIO`` -> Sets the trafic with TOP priority in case the ATC needs. Should be rarely used (Only shows in case CTOT exists).
    - ![#BE0000](https://img.shields.io/badge/-BE0000) ``SUSP`` - It would be set automatically when flightplan is suspended due to TOBT, TSAT or other cases from the server side.


- Column EvCTOT: It  show ctots provided by ctot file (ctot.txt).
  - Functions:
    - ``Add Event CTOT as MAN CTOT`` -> Set the CTOT of the tfc as a Manual CTOT (For events such as CTL or CTP) - (CTOT will only be set when CTOTs where TSAT>now+5min).
  - Colors:
    - ![#00c000](https://img.shields.io/badge/-00c000) ``GREEN`` -> Default.
