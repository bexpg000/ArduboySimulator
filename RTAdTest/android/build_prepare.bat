call app_info_setup.bat

::Update/write our local.properties file with our ANDROID NDK dir
call android update project -p ./
@echo on

:Get the emulator ready if it isn't, because it takes a freakin' long time to load
:start emulator %EMULATOR_AVD%

:Copy refresh resources, assuming the windows version had them built with update_media recently...

rmdir assets /S /Q
mkdir assets

rmdir libs /S /Q
del libs
mkdir libs


:It's ok if you get "0 files copied" messages, there just aren't files in some of these
mkdir assets\interface
xcopy ..\bin\interface assets\interface /E /F /Y

mkdir assets\game
xcopy ..\bin\game assets\game /E /F /Y

mkdir assets\audio
xcopy ..\bin\audio assets\audio /E /F /Y

:Kill old files handing around to avoid confusion
del libs\armeabi\lib%SMALL_PACKAGE_NAME%.so
rmdir bin /S /Q
rmdir gen /S /Q
rmdir temp_final_src /S /Q

:build temp src directory, which will preprocess and output to staging_src a bit later with ant

rmdir temp_src /S /Q
mkdir temp_src
mkdir temp_src\com
mkdir temp_src\com\%COMPANY_PACKAGE_NAME%
mkdir temp_src\com\%COMPANY_PACKAGE_NAME%\%SMALL_PACKAGE_NAME%

:move in our app specific java file(s)
xcopy src temp_src\com\%COMPANY_PACKAGE_NAME%\%SMALL_PACKAGE_NAME%  /E /F /Y

mkdir temp_final_src
mkdir temp_final_src\com

:copy pre processed shared java files over
xcopy ..\..\shared\android\v2_src\java temp_src\com\%COMPANY_PACKAGE_NAME%\%SMALL_PACKAGE_NAME% /E /F /Y

:copy any extra libraries we need over - skip the preprocessing step for these, move them directly to the final dir

:for IAP (must include, even if you don't use it)
echo d | xcopy ..\..\shared\android\optional_src\com\android temp_final_src\com\android /E /F /Y

:New file needed for v3 in app billing that google gives us
copy ..\..\shared\android\optional_src\libs\android libs

:there is a single .cpp file that we need to preprocess as well (to modify the jni function names to match our class path), C++ macros just can't do it
rmdir temp_final_cpp_src  /S /Q
call ant preprocess_cpp

:optional libs we made need, you need to download and put in the appropriate place if you use them

:for startapp   (comment out the goto if you want to use it)
goto skipstartapp;
set TEMPFILE=..\..\shared\android\optional_src\libs\startapp\SearchHelperService-1.0.14-jar-with-dependencies.jar
:Extra check to make sure we can locate the files
if exist "%TEMPFILE%" (
echo Located startapp files.
) else (
echo Cannot find the Startapp library files.  Download from startapp and place lib into shared\android\optional_src\libs\startapp
..\..\shared\win\utils\beeper.exe /p
)
copy %TEMPFILE% libs
:skipstartapp

:For Hooked/wasabi  (comment out the goto if you want to use it)
:goto skiphooked;
set TEMPFILE=..\..\shared\android\optional_src\libs\wasabi\wasabi.jar
:Extra check to make sure we can locate the files
if exist "%TEMPFILE%" (
echo Located Hooked/wasabi files.
) else (
echo Cannot find the Hooked/Wasabi library files.  Download from Hooked and place lib into shared\android\optional_src\libs\wasabi
..\..\shared\win\utils\beeper.exe /p
)
copy %TEMPFILE% libs
:skiphooked

:for Chartboost  (comment out the goto if you want to use it)
:goto skipchartboost;
set TEMPFILE=..\..\shared\android\optional_src\libs\ChartBoost\chartboost.jar

:Extra check to make sure we can locate the files
if exist "%TEMPFILE%" (
echo Located Chartboost files.
) else (
echo Cannot find the ChartBoost .jar file.  Download and place lib into shared\android\optional_src\libs\ChartBoost
..\..\shared\win\utils\beeper.exe /p
)
copy %TEMPFILE% libs
:skipchartboost

:for Flurry  (comment out the goto if you want to use it)
:goto skipflurry;
set TEMPFILE=..\..\shared\android\optional_src\libs\Flurry\FlurryAgent.jar

:Extra check to make sure we can locate the files
if exist "%TEMPFILE%" (
echo Located flurry files.
) else (
echo Cannot find FlurryAgent.jar.  Download and place lib into shared\android\optional_src\libs\Flurry
..\..\shared\win\utils\beeper.exe /p
)
copy %TEMPFILE% libs
:skipflurry

:for tapjoy (v9 and newer)  (comment out the goto if you want to use it)
:goto skiptapjoy;
set TEMPFILE=..\..\shared\android\optional_src\libs\Tapjoy\tapjoyconnectlibrary.jar

:Extra check to make sure we can locate the files
if exist "%TEMPFILE%" (
echo Located tapjoy files.
) else (
echo Cannot find tapjoyconnectlibrary.jar.  Download and place lib into shared\android\optional_src\libs\Tapjoy
..\..\shared\win\utils\beeper.exe /p
)
copy %TEMPFILE% libs
:skiptapjoy

:Next package it with the java part
call ant preprocess

:build the C/C++ parts.. you might want to remove/modify the -j7 part if you don't have a multicore processor with 8 threads...
call ndk-build -j7
if not exist libs/armeabi/lib%SMALL_PACKAGE_NAME%.so ..\..\shared\win\utils\beeper.exe /p