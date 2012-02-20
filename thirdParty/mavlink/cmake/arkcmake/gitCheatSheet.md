# git cheat sheet #

git is a distributed version control system (DVCS). Version control is useful for software development for many reasons. Each version, or commit, acts as history of the software. If something breaks, it is easy to go back to a previous version that worked. git is also very useful for writing software as a team. Each commit is labeled with an author, so other authors can see who made what change. 

git can be used in a variety of ways, but in this lab, we use a simple setup. The main, shared code is stored on either github (public, for open source projects), bitbucket (private), or our lab server (for projects that cannot be online). These repositories are considered "origin master". Users use the 'clone' command once to make a local copy of the repository, 'pull' to get updated changes, and 'commit' and 'push' to contribute their changes. 

Following is a list of basic commands for git. 

## Initial setup ##

To fetch a project for the first time (aka clone a repository):
_substitute appropriate address_

```console
git clone git@github.com:jgoppert/jsbsim.git
cd jsbsim
```

To setup your username and password (so everyone knows who did such great work!):

```console
git config --global user.name "Firstname Lastname"
git config --global user.email "your_email@youremail.com"
```

## Daily use ##

**Required:** Always do this before beginning any work to get any changes someone else may have made (this can avoid a lot of headaches):

```console
git pull
```

To see current un-committed changes: 

```console
git diff
```

**Required:** To add all new and changed files, then commit changes: 
(the only time you would omit push would be if you are working offline)

```console
git add .
git commit -a
git push
```

If you get an error from the push command: 

```console
git push origin master
```

## Intermediate use ##

To only commit changes to the file "new":
(example: you changed trim states but do not want to save them)

```console
git add new
git commit
git push
```

Reverting all changes since last commit:

```console
git reset --hard
```
## Using branches ##

A great idea if you think you might break everything!

```console
git branch experiment
git checkout experiment
```

To switch back to the main files: 

```console
git checkout master
```

To combine the code from the branch back into the main files (this may cause conflicts, which will be clearly marked in the file):

```console
git merge experiment
```
