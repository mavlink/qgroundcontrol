# Using Unix and vi #

A few basic Unix commands are all that is necessary to be able to work with files. 

First, all of these commands must be typed into the Terminal. Its icon is a black square with '>_' in white. On Debian, it is located at Applications > Accessories > Terminal. After typing the command, press Enter.  

'ls' lists the files in the current directory. The '-al' option shows hidden files and shows extra information in a list (directories have a d in front of them). Specifying a path will show the files in that directory.  

```console
ls
ls -al
ls -al Projects
```

'pwd' shows the absolute path to the current directory. 

```console
pwd
```

'cd' means change directory. It can be used in several ways. 

Used alone, it moves to the home directory of the current user. This is useful if you are looking at files somewhere else and need to go back to the home directory without typing the whole path. 

```console
cd
```

If there are directories inside the current directory (use 'ls -al', directories have d in front of them), you can move to them with a relative path:  

```console
cd Projects/jsbsim
```

If you want to move to a directory *outside* of the current directory, use an absolute path (the same thing that would be returned by 'pwd', notice that it begins with a slash): 

```console
cd /usr/local
```

Another important symbol is the tilde (~), which means the home directory of the current user (the same place 'cd' will move to):

```console
cd ~/Projects/jsbsim
```

'cp' is copy and 'mv' is move, which is also used to rename files.

To move a file from the current directory into the directory 'src/':

```console
mv testfile src/
```

To rename a file: 

```console
mv testfile release
```

To copy a file: 

```console
cp release test
```
