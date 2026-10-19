Blinks are supported under the Arduino IDE as a "custom" board package. The IDE finds it through the URL people add under `File->Preferences->Additional Boards Manager URLs`, which points at `package_move38.com-blinks_index.json`. That index file lists each release zip along with its exact size and SHA-256 checksum, so it has to be regenerated for every release. `make_release.py` does that for you.

# How to push a release to the Arduino Boards Manager

1. Set `version=X.Y.Z` in `platform.txt` to the new version and commit everything.
2. Run `python make_release.py X.Y.Z` from the top of the repo. It:
    * builds `Move38-Arduino-Platform-X.Y.Z.zip` from the last commit using `git archive`, so the zip has only committed files (no `.git` folder, no local build output), LF line endings, normal Unix permissions, and a single `Move38-Arduino-Platform/` top folder. Files and folders marked `export-ignore` in `.gitattributes` are left out.
    * adds a new entry at the top of `package_move38.com-blinks_index.json` with the right URL, size and checksum. Older entries are kept so people can still choose an earlier version in Boards Manager.
3. Commit the updated `package_move38.com-blinks_index.json` and push.
4. Create a new GitHub release whose tag is exactly `X.Y.Z` (the download URL in the index depends on it) and attach the zip.
5. Put the updated `package_move38.com-blinks_index.json` at the URL that people add to their Arduino IDE.

    * The current canonical URL in the Move38.com documentation is `https://boardsmanager.com/package_move38.com-blinks_index.json`, which is currently controlled by [me](https://josh.com/contact.html).
    * I also like to put the matching package JSON file into the release with the ZIP file as a reference, but it is not used for anything.

Once the package file at the canonical URL is updated, new users will get the updated release and existing users will be prompted to update next time they use the Arduino IDE (or they can manually get the update from inside the Boards Manager window).

## Testing a release before publishing

With [arduino-cli](https://arduino.github.io/arduino-cli/) (the same engine Arduino IDE 2.x uses), serve the folder containing the zip and a copy of the index whose `url` points at that local server, then:

```
arduino-cli core update-index --additional-urls http://127.0.0.1:8000/package_move38.com-blinks_index.json
arduino-cli core install move38:avr --additional-urls http://127.0.0.1:8000/package_move38.com-blinks_index.json
arduino-cli compile -b move38:avr:blink <path to an example sketch>
arduino-cli upload --dry-run -v -b move38:avr:blink <path to an example sketch>
```
