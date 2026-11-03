# Building TB-3030 with GitHub (Windows)

1. Create a **new** repository on GitHub, for example `TB-3030` (private is fine).
2. Unzip `TB-3030_v1.0_source.zip`.
3. In the repository, click **Add file > Upload files** and drag in **everything inside** the `TB-3030` folder. Then click **Commit changes**.
   - It doesn't matter if folders get flattened. The build finds the files wherever they are.
4. **Check the workflow file.** GitHub often skips the hidden `.github` folder.
   - If `.github/workflows/build.yml` is missing, click **Add file > Create new file**.
   - Type exactly `.github/workflows/build.yml` as the name.
   - Paste in the content of `build.yml` (also supplied separately) and click **Commit changes**.
5. Go to **Actions**. **Build TB-3030 v1** should start by itself. If it doesn't, click it in the left-hand list and choose **Run workflow**.
   - If workflows are disabled, click the green *"I understand my workflows..."* button.
6. The first step prints `=== TB-3030 workflow v1 ===` and ends with **All files found.** If a file is reported missing, upload it.
7. The build takes about 15-30 minutes. When it's green, scroll to **Artifacts** and download:
   - **TB-3030-Windows-Setup**, which contains the installer `TB-3030-1.0.0-Windows-Setup.exe`
   - **TB-3030-Windows-Portable**, which contains `TB-3030.exe` and `TB-3030.vst3` to copy wherever you like

## Installing
- Run the setup. It installs for your user by default, needs no admin rights and doesn't try to close any programs (the McAfee issue we had with DR-1010).
- If you choose **Install for all users**, the VST3 goes to `C:\Program Files\Common Files\VST3`. For your user only, it goes to `%LOCALAPPDATA%\Programs\Common\VST3`.
- If your DAW (L-DAW, Reason...) doesn't find it, copy `TB-3030.vst3` from the portable package to `C:\Program Files\Common Files\VST3` and rescan.
- If Windows says "Windows protected your PC", choose **More info > Run anyway**. This appears because the app isn't code-signed.

## First start (stand-alone)
- Choose **Options > Audio/MIDI Settings**, pick your output (ASIO if the SDK was available at build time) and tick your MIDI keyboard.

## If the build fails
Send me the lines containing `error C` (compiler) or `ERROR:` (file check).
