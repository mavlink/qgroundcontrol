Note: 

  If you are using qextserialport-XXX.tar.gz, the qesp.qch and
  html files have been provided.

  Open the file "html/index.html" using your web browser.
  Or integrated the "html/qesp.qch" into your QtCreator.


== How to generate help files? ==

Simply run following commands at toplevel directory.
    qmake
    make docs

Or run the following command at this directory
    qdoc3 qextserialport.qdocconf

Then a folder called "html" will be generated. 
Open the file "html/index.html" using your web browser.

== How to integrated into Qt Creator or Qt Assistant? ==

Once the html files are generated. run following commands
   cd doc/html
   qhelpgenerator qesp.qhp

A file called "qesp.qch" will be generated.

For Qt Assistant: 
   Edit ==> Preferences ==>  Documentations ==> Add...

For Qt Creator
   Tools ==> Options ==> Help ==> Documentations ==> Add...
 
