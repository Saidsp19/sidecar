/*!
\file  geoStarsExcel.h
\brief This file documents the Microsoft Excel file that contains macros to link to
                geoStarsLib.dll. This file tests some of the functions in geoStarsLib by
                exercising site conversion information.

\section xls Excel functions available through the DLL

The file geo.xls contains a Microsoft Excel spreadsheet employing some of the functions
of geoStarsLib. It is meant as an example of how to call the DLL functions directly from Excel.

Here are the steps to use this file and/or incorporate the functions into your own spreadsheet.

  1. Load geoStars.bas into Excel via the Visual Basic Editor

        - Choose Tools->Macros->Visual Basic Editor
\image html geoExcelMenu.jpg "Opening the VB editor"

  - From the editor choose File->Import File
\image html geoExcelImport.jpg "Importing the gelStars.bas file"

  2. call the functions directly. These functions are used in this example:
        - geoDms2DD()  - Degrees Minutes Seconds to Decimal Degrees conversion
        - geoLlh2E()   - Latitude, Longitude, and Height converted to E component of earth fixed coordinates
        - geoLlh2F()   - Latitude, Longitude, and Height converted to F component of earth fixed coordinates
        - geoLlh2G()   - Latitude, Longitude, and Height converted to G component of earth fixed coordinates
        - geoMagGetDecNow() - get the current magnetic north of a site
        - geoSunNowAz() - get the azimuth angle of the sun at the current location and time
        - geoSunNowEl() - get the elevation angle of the sun at the current location and time
        - geoLlh2DiffX() - Latitude, Longitude, and Height converted to East component of local plane (tangential)
        - geoLlh2DiffY() - Latitude, Longitude, and Height converted to North component of local plane (tangential)
        - geoLlh2DiffX() - Latitude, Longitude, and Height converted to Up component of local plane (tangential)
        - geoXyz2A() - convert the East, North, and Up (X,Y,Z) of a site to polar coordinates - Azimuth
        - geoXyz2E() - convert the East, North, and Up (X,Y,Z) of a site to polar coordinates - Elevation
        - geoXyz2R() - convert the East, North, and Up (X,Y,Z) of a site to polar coordinates - Range

Most of these calls are condensed into structured in the main library. However, breaking these out into
separate components allows Excel to call them directly by mere mortals.

\image html geoExcel.jpg "Excel test program"



*/
