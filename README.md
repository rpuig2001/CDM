# CDM plugin V2
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
- If there is no master controller, you should use the command ``.cdm master {airport}`` of the airport you want to become the master. You can have as many airport as you want, but there can ony be **1 MASTER at the same time** (The MASTER should be DELIVERY or the lowest ATC position to have access to all CDM actions).
- Add The following items to the departure list with their actions:

- EOBT

![image](https://i.gyazo.com/928f2e35f0a4248e17442bba552d72e0.png)

- E

![image](https://i.gyazo.com/436e8eb7b20b00d2c39a483319d03425.png)

- TOBT

![image](https://i.gyazo.com/ad6344055e7de91ab8a386f7153d19e1.png)

- TSAT

![image](https://i.gyazo.com/37b0ad531fc4a32dfaffbf7db83c5546.png)

- TTOT

![image](https://i.gyazo.com/4533873ef8d8342cb5b35ed381bb0f47.png)

- TSAC

![image](https://i.gyazo.com/8f9d55ec477a8c21ddb63df3b4da15a1.png)

- ASAT

![image](https://i.gyazo.com/8fb33ba38fb68e48de64b0139322e466.png)

- ASRT

![image](https://i.gyazo.com/54c3956f46f63ee3b44e84308bb6fe5d.png)

- CTOT

![image](https://i.gyazo.com/775e1bf69fac29e2e3a776d35e67952a.png)


## MASTER AND SLAVE:
- Master: The master is the "admin" of the CDM and is the only controller who calculates the times (TSAT, TTOT and ASRT)
  - Use ``.cdm master {airport}`` command (**TO LET THE CDM DO IT'S JOB, ONLY 1 CONTROLLER CAN BE THE MASTER AT THE SAME TIME**).
- Slave: The Slave Monitors the CDM and has some limited actions.
  - Default type, so, you don't need to change anything unless you are now a master, where you can use ``.cdm slave {airport}`` command.

### HOW TO DO A CONTROLLER CHANGE CORRECTLY:
1. Check to have the same *CDMconfig.xml* and *taxizones.txt* configuration, otherwise it won't work correctly.
2. The **Old controller** changes to Slave with command ``.cdm slave``.
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
  - Select CTOT option (``ctot option="callsign or cid"``):
    - "callsign": It gets CTOT if the callsign is the same as the defined in ctot.txt (ex. defaultRate option="VLG11A,2213").
    - "cid": It gets CTOT if the cid is the same as the defined in ctot.txt (ex. defaultRate option="XXXXXX,2213").
  - Normal Visibility Operations Rate/hour (ex. rate ops="40").
  - Low Visibility Operations Rate/hour (ex. rateLvo ops="10").
  - Expired CTOT time, it selects the time before expire the CTOT if the pilot is not connected (ex. expiredCtot time="15").
  - ReaMsg (ex. minutes="0"). - It sets the time to add for the *"Send Rea Message"* function.
  - [OPTIONAL] Taxizones URL (ex. Taxizones url="https://........"), if no URL needed, just leave it blank (ex. Taxizones url="").
  - [FUTURE 2.0.8] [OPTIONAL] Ctots URL (ex. Ctot url="https://........"), if no URL needed, just leave it blank (ex. Ctot url="").
  - Default Taxi time in minutes if taxi time not found in the taxizones.txt file (ex. DefaultTaxiTime minutes="15").
  - Refresh Time in seconds (ex. RefreshTime seconds="20").
  - Debug mode activated (true) or desactivated (false) (ex. Debug mode="false" or Debug mode="true").
  - [OPTIONAL] FlowRestrictions URL to the JSON file - Format is defined below (ex. FlowRestrictions url:"https://...."), if no URL needed, just leave it blank (ex. FlowRestrictions url="").
  - [OPTIONAL] FTP host to push CDM Data (ex. ftpHost host:"ftp.aaaaaa.com") - leave it blank if not in use "".
  - [OPTIONAL] FTP user to push CDM Data (ex. ftpUser user:"username") - leave it blank if not in use "".
  - [OPTIONAL] FTP password to push CDM Data (ex. ftpPassword password:"&&&&&&") - leave it blank if not in use "".
 
### ctot.txt
  - Add CTOTs which will be imported on Euroscope start-up or with the command ".cdm ctot". Add CTOTs with the following format: ``CALLSIGN,CTOT`` or ``XXXXXX,CTOT``, ex: ``XXXXXX,1745`` - XXXXXX is vatsim user's CID or ``VLG11P,1745`` (Each line has an aircraft)
 
### taxizones.txt
  - You can define a zone with an specific taxiTime with the following specifications ``AIRPORT:RUNWAY:BOTTOM_LEFT_LAT:BOTTOM_LEFT_LON:TOP_LEFT_LAT:TOP_LEFT_LON:TOP_RIGHT_LAT:TOP_RIGHT_LON:BOTTOM_RIGHT_LAT:BOTTOM_RIGHT_LON:TAXITIME``, ex:``LEBL:25L:41.286876:2.067318:41.290236:2.065955:41.295688:2.082523:41.292662:2.084613:10``, if no taxizone defined, the default taxi time is set to 15 min.

### rate.txt
  - You can set the rate/hour for specific runway and airport, if not declared, **AIRPORT WILL NOT BE CONSIDERED A CDM AIRPORT**. You can declare every runway rate with the following format: ``AIRPORT:RUNWAY=NormalRate_LvoRate``, ex:``LEBL:25L=40_20`` (Each line has a runway with his rate)

*Examples can be found in the givenfiles.*

## Flow Restriction
### How does it work?
Flow restrictions create CTOTs to planes afected with the published restrictions (Only MDIs Available for now).

### JSON format
Each Restriction must have:
  - ```TIME```: Time interval for MDI (number value, ex. 3).
  - ```DEPA```: Departure Aerodrome for flights affected or "ALL" for all flights. (ex. "LEBL" or "ALL").
  - ```DEST```: Destination Aerodrome for flights affected or "ALL" for all flights. (ex. "LEPA" or "ALL").
  - ```VALIDDATE```: Valid date ```day/month``` (ex. "07/04").
  - ```VALIDTIME```: Valid date ```start-end``` (ex. "1700/1900").
  - ```MESSAGE```: Message to show in CDM's flow message field (ex. "message").
  
[FUTURE 2.0.8] Each Restriction must have:
  - ```TIME```: Time interval for MDI (number value, ex. 3).
  - ```DEPA```: Departure Aerodrome for flights affected. (ex. "LEBL", "LE**" or "****" for all airports).
  - ```DEST```: Destination Aerodrome for flights affected. (ex. "EDDF", "ED**" or "****" for all airports).
  - ```VALIDDATE```: Valid date ```day/month``` (ex. "07/04").
  - ```VALIDTIME```: Valid date ```start-end``` (ex. "1700/1900").
  - ```MESSAGE```: Message to show in CDM's flow message field (ex. "message").

Example:
```
{
    "MDI": [
      {
        "TIME": 3,
        "DEPA": "ALL",
        "DEST": "LPPT",
        "VALIDDATE": "07/04",
        "VALIDTIME": "1700-1900",
        "MESSAGE": "LPPT Thursdays"
       },
       {
        "TIME": 3,
        "DEPA": "ALL",
        "DEST": "EDDH",
        "VALIDDATE": "07/04",
        "VALIDTIME": "1700-1900",
        "MESSAGE": "EDDH Thursday"
       }
    ]
}
```
[FUTURE 2.0.8] Example:
```
{
    "MDI": [
	{
        "TIME": 3,
		"DEPA": 
			[
				"LE**"
			],
		"DEST": 
			[
				"ED**",
				"EB**",
           	 		"LS**",
				"EHAM",
				"****"
			],
		"VALIDDATE": "03/08",
		"VALIDTIME": "1000-2300",
		"MESSAGE": "Fly Out Spain"
        }
	]
}
```

## FTP files and format
### Files
Every airport will have a different txt file (ex. LEBL airport: CDM_data_LEBL.txt)

### Format
``CALLSIGN,EOBT,TSAT,TTOT,CTOT,FlowRestrictionMessage,``
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

## Commands
- ``.cdm reload`` - Reloads all CDM plugin configs and taxizones file.
- ``.cdm refresh`` - Force the refresh phase to do it now.
- [DISABLED] ``.cdm ctot`` - Loads ctot.txt data.
- ``.cdm save`` - Saves data to savedData.txt to sync times with other controllers.
- ``.cdm load`` - Loads savedData.txt to sync times with other controllers.
- ``.cdm master {airport}`` - Become the master of the selected airport.
- ``.cdm slave {airport}`` - Turn back to slave of the selected airport.
- ``.cdm refreshtime {seconds}`` - It changes the refresh rate time in seconds (Default 30, MAX 99 Seconds).
- [Euroscope May crash (Checking it out for further versions)] ``.cdm delay {minutes}`` - Adds delay minutes to all traffics.
- ``.cdm lvo on`` - Activates lvo using the defined lvo rate in rate.txt file.
- ``.cdm lvo off`` - Desactivates lvo rate.
- ``.cdm help`` - Sends a message with all commands.

## Functions and colors:
- Column A: It toggles an A to remember the controller that the plane is waiting for something.
  - ![#f5ef0d](https://via.placeholder.com/15/f5ef0d/000000?text=+) `YELLOW` or defined ``color5``: Always this color.

- Column EOBT: It gets the EOBT set by the pilot in the flightplan.
  - Color defined as ``color8``.

- Column TOBT: If EventMode is disabled, We can not simulate it, so it gets the EOBT and the colors is green. Otherwise TOBT will be functional and will calculate TSAT and TTOT from the TOBT time. To delete it simple edit the time and press enter deleting the content.
  - ![#8fd894](https://via.placeholder.com/15/8fd894/000000?text=+) `LIGHT GREEN` or defined ``color2``: From EOBT-35 to EOBT-5.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`  or defined ``color1``: After EOBT-5.

- Column E: It shows a letter depending on the plane timmings:
  - P: EOBT is farther than the Actual Time - 35min.
  - C: EOBT is less than 35min and TOBT hasn't expired (TOBT+6) or TSAT hasn't expired (TSAT+6).
  - I: TSAT has expired.

- Column TSAT: It is the TTOT - the taxi time defined in the taxizones.txt, otherwise it sets 15min.
  - ![#8fd894](https://via.placeholder.com/15/8fd894/000000?text=+) `LIGHT GREEN` or defined ``color2``: From EOBT-35 to TSAT-5 and after TSAT+6 if not expired.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`  or defined ``color1``: From TSAT-5 to TSAT+5.
  - ![#f5ef0d](https://via.placeholder.com/15/f5ef0d/000000?text=+) `YELLOW` or defined ``color5``: From TSAT+5 to TSAT+6.

- Column TTOT: The plugin calculates a TSAT based on this column, the TTOT, you can't have planes with same TTOT, the time between departures is calculated from the rate/hour. So if you need 40 departures/hour, the plugin will calculate it for you with no equal TTOTs.
  - Color defined as ``color9``.

- Column TSAC: With the left click you can directly set the tsat and with the right click you can remove it or set the time you want. If this field is +/- 5min that the TSAT, the color change to orange to indicate that his TSAT has changed more than 5min.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN` or defined ``color1``: If between +/- 5min of TSAT.
  - ![#ed852e](https://via.placeholder.com/15/ed852e/000000?text=+) `ORANGE` or defined ``color4``: If +/- 5min of TSAT.

- Column ASAT: It sets the time when ST-UP, TAXI or DEPA state is set on the first time.
  - ![#00c000](https://via.placeholder.com/15/00c000/000000?text=+) `DARK GREEN`  or defined ``color1``: If actual time < ASAT - 5min.
  - ![#f5ef0d](https://via.placeholder.com/15/f5ef0d/000000?text=+) `YELLOW` or defined ``color5``: From ASAT+5 to always.

- Column ASRT: It shows the requested StartUp time, It can be added to the list with the toggle function or sending a REA Msg.
  - Color defined as ``color10``.

- Column CTOT: It shows aircraft's CTOT which can be added, modified, removed or reloaded.
  - Color defined as ``color11``.
