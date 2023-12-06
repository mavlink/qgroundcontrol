# Initial Repository Setup For Custom Build

The suggested mechanism for working on QGC and a custom build version of QGC is to have two separate repositories. The first repo is your main QGC fork. The second repo is your custom build repo.

## Main QGC Respository

This repo is used to work on changes to mainline QGC. When creating your own custom build it is not uncommon to discover that you may need a tweak/addition to the custom build to achieve what you want. By discussing those needed changes firsthand with QGC devs and submitting pulls to make the custom build architecture better you make QGC more powerful for everyone and give back to the community.

The best way to create this repo is to fork the regular QGC repo to your own GitHub account.

## Custom Build Repository

This is where you will do your main custom build development. All changes here should be within the custom directory as opposed to bleeding out into the regular QGC codebase.

Since you can only fork a repo once, the way to create this repo is to "Create a new repository" in your GitHub account. Do not add any additional files to it like gitignore, readme's and so forth. Once it is created you will be given the option to setup up the Repo. Now you can select to "import code from another repository". Just import the regular QGC repo using the "Import Code" button.
