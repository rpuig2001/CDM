# CDM plugin
CDM is an Euroscope plugin based on the real life CDM tool that allows us to improve the departure flows at airports.
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
- Add The following items to the departure list with their actions:

- A

![image](https://i.gyazo.com/e7bae0d995a6f77b0e3b7fed115d854e.png)

- EOBT

![image](https://i.gyazo.com/e831b09bc5a75a8971bcae892ef16940.png)

- E

![image](https://i.gyazo.com/3c65d71bc812ccf6966c4694c9fa425d.png)

- TOBT

![image](https://i.gyazo.com/754f328e3d0fb087077cd2bfc89c1a54.png)

- TSAT

![image](https://i.gyazo.com/f4de2894de5f5b12733ad94896d9cdbb.png)

- TTOT

![image](https://i.gyazo.com/931cb1578776283261382f3735fea5e7.png)

- TSAC

![image](https://i.gyazo.com/6a28ed89ecfe8aab259d533febe45000.png)

- ASAT

![image](https://i.gyazo.com/ae930f513a05d85a9776631ff89fbf7d.png)

- ASRT

![image](https://i.gyazo.com/f320a807ee5f8f35c63cac38d671f1a2.png)

- CTOT

![image](https://i.gyazo.com/fa2d4b6bc87832d2983bf6800bdb824d.png)


## MASTER AND SLAVE:
- Master: The master is the "admin" of the CDM and is the only controller who calculates the times (TSAT, TTOT and ASRT)
  - Use ``.cdm master`` command (**TO LET THE CDM DO IT'S JOB, ONLY 1 CONTROLLER CAN BE THE MASTER AT THE SAME TIME**).
- Slave: The Slave Monitors the CDM and is unable to add/edit CTOTs.
  - Default type, so, you don't need to change anything unless you are now a master, where you can use ``.cdm slave`` command.

## HOW TO DO A CONTROLLER CHANGE CORRECTLY:
1. Check to have the same *CDMconfig.xml* and *taxizones.txt* configuration, otherwise it won't work correctly.
2. The **Old controller** changes to Slave with command ``.cdm slave``.
3. Once there are no master controllers, the **new controlles** gets the master "rol" with the command ``.cdm master``.
4. That's it!

### Define configurations
- Colors.txt
  - color1 = GREEN
  - color2 = GREENNOTACTIVE
  - color3 = GREY
  - color4 = ORANGE
  - color5 = YELLOW
  - color6 = DARKYELLOW
  - color7 = RED
- CDMconfig.xml
  - Add icao (ex. apt icao="LEPA").
  - Select rate option:
    - "0": It uses the specified rate in the rate.txt file for each runway, if not defined, it will get the default rate ops selected in the "rate ops" field (ex. defaultRate option="0").
    - "1": It uses the default rate selected in the "rate ops" field for each rwy (ex. defaultRate option="1").
  - Normal Visibility Operations Rate/hour (ex. rate ops="40").
  - Low Visibility Operations Rate/hour (ex. rateLvo ops="20").
  - Expired CTOT time, it selects the time before expire the CTOT if the pilot is not connected (ex. expiredCtot time="15").
  - ReaMsg (ex. minutes="0"). - It sets the time to add for the *"Send Rea Message"* function.
- ctot.txt
  - Add CTOTs which will be imported on Euroscope start-up or with the command ".cdm ctot". Add CTOTs with the following format: ``CALLSIGN,CTOT``, ex:``VLG11P,1745`` (Each line has an aircraft)
- taxizones.txt
  - You can define a zone with an specific taxiTime with the following specifications ``AIRPORT:RUNWAY:BOTTOM_LEFT_LAT:BOTTOM_LEFT_LON:TOP_LEFT_LAT:TOP_LEFT_LON:TOP_RIGHT_LAT:TOP_RIGHT_LON:BOTTOM_RIGHT_LAT:BOTTOM_RIGHT_LON:TAXITIME``, ex:``LEBL:25L:41.286876:2.067318:41.290236:2.065955:41.295688:2.082523:41.292662:2.084613:10``, if no taxizone defined, the default taxi time is set to 15 min.
  - rate.txt
    - You can set the rate/hour for specific runway to use with the option ``defaultRate option="1"`` in the CDMconfig.xml. You can declare every runway rate with the following format: ``AIRPORT:RUNWAY=RATE``, ex:``LEBL:25L=40`` (Each line has a runway with his rate)

*Examples can be found in the given CDMconfig.xml and taxizones.txt file.*

### Commands
- ``.cdm reload`` - Reloads all CDM plugin.
- ``.cdm rate {rate}`` - Sets the selected rate.
- ``.cdm airport {icao}`` - Sets the selected airport.
- ``.cdm ctot`` - Loads ctot.txt data.
- ``.cdm save`` - Saves data to savedData.txt to sync times with other controllers.
- ``.cdm load`` - Loads savedData.txt to sync times with other controllers.
- ``.cdm lvo`` - It sets the configured rate for *LOW VISIBILITY OPERATIONS* in the CDMconfig.xml
- ``.cdm nvo`` - It sets the configured rate for *NORMAL VISIBILITY OPERATIONS* in the CDMconfig.xml

## Functions and colors:
- Column A: It toggles an A to remember the controller that the plane is waiting for something.
  - ![#f5ef0d](https://via.placeholder.com/15/f5ef0d/000000?text=+) `YELLOW`: Always this color.

- Column EOBT: It gets the EOBT set by the pilot in the flightplan.
  - ![#b6b6b6](https://via.placeholder.com/15/b6b6b6/000000?text=+) `GREY`: Always this color.

- Column TOBT: We can not simulate it, so it gets the EOBT and the colors is green.
  - ![#8fd894](https://via.placeholder.com/15/8fd894/000000?text=+) `LIGHT GREEN`: From EOBT-35 to EOBT-5.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`: After EOBT-5.

- Column E: It shows a letter depending on the plane timmings:
  - P: EOBT is farther than the Actual Time - 35min.
  - C: EOBT is less than 35min and TOBT hasn't expired (TOBT+6) or TSAT hasn't expired (TSAT+6).
  - I: TSAT has expired.

- Column TSAT: It is the TTOT - the taxi time defined in the taxizones.txt, otherwise it sets 15min.
  - ![#8fd894](https://via.placeholder.com/15/8fd894/000000?text=+) `LIGHT GREEN`: From EOBT-35 to TSAT-5 and after TSAT+6 if not expired.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`: From TSAT-5 to TSAT+5.
  - ![#f5ef0d](https://via.placeholder.com/15/f5ef0d/000000?text=+) `YELLOW`: From TSAT+5 to TSAT+6.

- Column TTOT: The plugin calculates a TSAT based on this column, the TTOT, you can't have planes with same TTOT, the time between departures is calculated from the rate/hour. So if you need 40 departures/hour, the plugin will calculate it for you with no equal TTOTs.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`: Always this color.

- Column TSAC: With the left click you can directly set the tsat and with the right click you can remove it or set the time you want. If this field is +/- 5min that the TSAT, the color change to orange to indicate that his TSAT has changed more than 5min.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`: If between +/- 5min of TSAT.
  - ![#ed852e](https://via.placeholder.com/15/ed852e/000000?text=+) `ORANGE`: If +/- 5min of TSAT.

- Column ASAT: It sets the time when ST-UP, TAXI or DEPA state is set on the first time.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`: If actual time < ASAT - 5min.
  - ![#f5ef0d](https://via.placeholder.com/15/f5ef0d/000000?text=+) `YELLOW`: From ASAT+5 to always.

- Column ASRT: It shows the requested StartUp time, It can be added to the list with the toggle function or sending a REA Msg.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`: Always this color.

- Column CTOT: It shows aircraft's CTOT which can be added, modified or removed.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`: Always this color.