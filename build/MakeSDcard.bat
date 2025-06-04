echo "This bat file will download the essential files for the SD card"
set /p DRIVE=Enter SD card drive letter (including the ':') 

cd %DRIVE%

powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/config.txt', 'config.txt')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/logo4.jpg', 'logo4.jpg')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/vconfig.txt', 'vconfig.txt')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/loading.jpg', 'loading.jpg')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/v3small.jpg', 'v3small.jpg')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/vicback.jpg', 'vicback.jpg')"
md edit
cd edit
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/edit/ace.js', 'ace.js')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/edit/jquery.min.js', 'jquery.min.js')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/NMEADisplay/SdRoot/edit/index.htm', 'index.htm')"
cd..
dir /s 
echo "Finished press any key to exit"
Pause
