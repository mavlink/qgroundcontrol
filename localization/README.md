To generate (or update) a source translation file, use the `to-crowdin.sh` script in this directory. You will need to update the path to Qt within it.

This will parse all the source files and generate a language translation file called qgc.ts, which should be uploaded to crowdin.

Once translations have been done/updated, within Crowdin "Build and Download". Extract the resulting qgroundcontro.zip here and run `from-crowdin.py`.
