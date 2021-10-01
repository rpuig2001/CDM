# CDM plugin
CDM is an Euroscope plugin based on the real life CDM tool that allows us to improve the departure flows at airports.
CDM includes the following times:
- EOBT: Estimated off block time.
- TOBT: Target off block time.
- TSAT: Target Start-Up Approval Time.
- TTOT: Target Take Off Time.
- TSAC: Target Start-Up Approval Communicated.
- ASRT: Actual Start-Up Request Time.


## How to use:
- Load up the plugin.
- Add The following items to the departure list with their actions:

- A

![image](https://i.gyazo.com/2a15bf80068f46e01f48fee6b3ef97e0.png)

- EOBT

![image](https://i.gyazo.com/83cebfb8543e58eca823b7a5a92fa3fa.png)

- E

![image](https://i.gyazo.com/3c65d71bc812ccf6966c4694c9fa425d.png)

- TSAT

![image](https://i.gyazo.com/754f328e3d0fb087077cd2bfc89c1a54.png)

- TTOT

![image](https://i.gyazo.com/f4de2894de5f5b12733ad94896d9cdbb.png)

- TSAT

![image](https://i.gyazo.com/bfc39b46e67f53683236020c8fe57ea2.png)

- TSAC

![image](https://i.gyazo.com/1d3792af2947ddae6bcde5075abb9582.png)

- ASRT

![image](https://i.gyazo.com/47ca5006438009d4fa573ba328cc0abb.png)


### Define configurations
- CDMconfig.xml
  - Add icao (ex. apt icao="LEAL").
  - Rate/hour (ex. rate ops="40").
- taxizones.txt
  - You can define a zone with an specific taxiTime with the following specifications ``AIRPORT:RUNWAY:BOTTOM_LEFT_LAT:BOTTOM_LEFT_LON:TOP_RIGHT_LAT:TOP_RIGHT_LON:TAXITIME``, ex:``LEPA:24R:39.543504:2.712383:39.548777:2.719502:10``, if no taxizone defined, the default taxi time is set to 15 min.

*Examples can be found in the given CDMconfig.xml and taxizones.txt file.*


## Functions and colors:
- Column A: It toggles an A to remember the controller that the plane is waiting for something.

- Column EOBT: It gets the EOBT set by the pilot in the flightplan.

- Column TOBT: We can not simulate it, so it gets the EOBT and the colors is green.

- Column E: It shows a letter depending on the plane the status:
  - P: 
  - C: 
  - I: 

- Column TSAT: It is the TTOT - the taxi time defined in the taxizones.txt, otherwise it sets 15min.

- Column TTOT: No same TTOTs

- Column TSAC: With the left click you can directly set the tsat and with the right click you can remove it or set the time you want. If this field is +/- 5min that the TSAT, the color change to orange to indicate that his TSAT has changed more than 5min.

- Column ASRT: It sets the time when ST-UP, TAXI or DEPA state is set on the first time.