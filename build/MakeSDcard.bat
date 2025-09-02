echo "This bat file will download the essential files for the SD card"
set /p DRIVE=Enter SD card drive letter (including the ':') 

%DRIVE%
md edit
md logs
dir /s 
echo "press any key to continue"
pause


powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/config.txt', '%DRIVE%/config.txt')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/logo4.jpg', '%DRIVE%/logo4.jpg')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/vconfig.txt', '%DRIVE%/vconfig.txt')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/loading.jpg', '%DRIVE%/loading.jpg')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/v3small.jpg', '%DRIVE%/v3small.jpg')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/vicback.jpg', '%DRIVE%/vicback.jpg')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/colortest.txt', '%DRIVE%/colortest.txt')"

powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/edit/ace.js', '%DRIVE%/edit/ace.js')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/edit/jquery.min.js', '%DRIVE%/edit/jquery.min.js')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/edit/index.htm', '%DRIVE%/edit/index.htm')"
cd..
dir /s 
echo "Finished press any key to exit"
Pause
