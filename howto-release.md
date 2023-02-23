Blinks are supported under the Arduino IDE as a "custom" board package. If you just want to develop software for the blinks platform, the easiest way is to let the Arduino IDE handle installing and updating this custom board package. The IDE finds custom board package using the URLs set in `File->Preferences->Additional Board Manager URLs`. Unfortunately it is not just as easy as putting the actual updated package at that URL (it should be, or at least have that file contains a list of URLS to packages). The IDE package file format requires obnoxious redundant data to also be encoded in that file. This HOWTO decribes how to create or update the package file pointed to by one of these URLs to point to a release of this repo.

# How to push a release to the Arduino boards manager

1. Zip up the contents of this repo, with the base path starting with `/Move38-Arduino-Platform`. 
 
     * You can save a lot of space by not including the `.git` folder, which is not needed for deployment. 
     * You can save a tiny bit of room by removing this file, which is not needed for deployment. :)
2. Create a new release on Github with an incremented version number. 
3. Add the new ZIP file to this release. 
4. Update the `package_move38.com-blinks_index.json` file to reflect this new ZIP.

    1. The URL should point to this new ZIP file. I like point to the blob in the github release.
    2. The size must be precise to the byte. You can get the size by inspecting the properties of the file or doing `dir` on a Windows command line.
    3. You can get the SHA256 checksum by uploading the file [here](https://emn178.github.io/online-tools/sha256_checksum.html) or running this command...
`certutil -hashfile "Move38-Arduino-Platform-main.zip" SHA256`
...from a Windows command line. 

5. Put the updated `package_move38.com-blinks_index.json` at the URL that people will then add to their Arduino IDE under `File->Preferences->Additional Board Manager URLs`.

    * The current canonical URL for in the Move38.com documentation is `https://boardsmanager.com/package_move38.com-blinks_index.json`, which is currently controlled by [me](https://josh.com/contact.html).
    * I also like to put the matching package JSON file into the release with the ZIP file as a reference, but it is not used for anything. 

Once the package file at the conanical URL is updated, new users will get the updated release and existing users will be prompted to update next time they use the Arduino IDE (or they can manually get the update from inside the board manager window). 
