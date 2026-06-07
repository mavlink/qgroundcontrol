# Qt Application Examples Template

Use the 'app-examples-template.qdoc' file when creating or updating any of
the Qt application examples.

The purpose of the template is to make it easier for technical writers and
developers to create documentation with a consistent look and feel. The
consistent look and feel also enhances user experience. For this to work, you
cannot move the sections around. You can and must change the text within the
angle brackets and check the links.

## To use the template

1. Copy the file to the `doc/src` folder in your documentation project.
2. Change the text within angle brackets to fit your example.

### Application examples structure

   - State the objective of the example.
   - Explain which Qt technologies are used.
   - Explain what Qt features are shown.

   - (Optional) Create a workflow diagram for more complicated examples.
   The [WebEngine Push Notifications Example](https://doc.qt.io/qt-6/qtwebengine-webenginewidgets-push-notifications-example.html)
   demonstrates this well.

   [QUIP-21](https://contribute.qt-project.org/quips/21) explains how to use
   images in Qt Documentation.

### Explain how to run the example

   - Include the [Launching Examples Template](https://github.com/qt/qtbase/blob/dev/doc/global/includes/examples-run.qdocinc).
   - Explain the expected application output after running the example.

### (Optional) Provide relevant platform information

   - List any platform limitations or exceptions, if applicable.

### (Optional) Provide a UI walkthrough

   - Use for more complicated examples.
   - The walkthrough describes how to navigate and access different parts of the example/UI.
   - The [Coffee Machine](https://doc.qt.io/qt-6/qtdoc-demos-coffee-example.html)
    example demonstrates this well.

### (Optional) Provide a list of the main Qt classes and modules the examples uses

   - Use for more complicated examples.
   - The [Bluetooth Low Energy Heart Rate Game](https://doc.qt.io/qt-6/qtbluetooth-heartrate-game-example.html)
    demonstrates this well.

#### (Optional) Create a class diagram

   - Use for more complicated examples.
   - The class diagram visually depicts the hierarchy of the Qt classes used in the example.
   - The [WebEngine Widgets Simple Browser Example](https://doc.qt.io/qt-6/qtwebengine-webenginewidgets-simplebrowser-example.html)
    demonstrates this well.

### Describe Feature A

   - Explain the implementation of the feature in the code.

### Feature B... (Add a separate feature section for all relevant features)

   - Explain the implementation of the feature in the code.

### (Optional) Include Squish testing information if applicable

   - If example was tested with Squish, include squish-tested-example.qdocinc

### (Optional) Create a Best practices section
   - Add any relevant best practices.

### Add links to relevant files
   - Use \sa command to link to All Qt Examples, and to other relevant documentation.
   - A link to the source code is automatically generated whenever the \example command is used.
