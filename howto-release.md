# How to push a release to the Arduino boards manager

1. Zip up the contents of this repo, with the base path starting with `/Move38-Arduino-Platform`. 
 
     * You can save a lot of space by not including the `.git` folder, which is not needed for deployment. 
     * You can save a tiny bit of room by removing this file, which is not needed for deployment. :)
2. Create a new release on Github with an incremented version number. 
3. Add the new ZIP file to this release. 
4. Update the `package_move38.com-blinks_index.json` file to reflect this new ZIP.

    1. The URL should point to this new ZIP file. I like point to the blob in the github release.
    2. The size must be precise to the byte. You can get the size by just inspecting the properties of the file.
    3. You can get the SHA256 checksum by uploading the file [here](https://emn178.github.io/online-tools/sha256_checksum.html).

5. Put the updated `package_move38.com-blinks_index.json` at the URL that people will then add to thier Arduino IDE under `File->Preferences->Additional Board Manager URLs`.

    * The current canonical URL for in the Move38.com documentation is `https://boardsmanager.com/package_move38.com-blinks_index.json`, which is currently controlled by [me](https://josh.com/contact.html).

Once the package file is updated, new users will get the updated release and existing users will be prompted to update next time they use the Arduino IDE. 
