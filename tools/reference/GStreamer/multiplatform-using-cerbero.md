# Multiplatform deployment using Cerbero

Cerbero is the build and packaging system used to construct
GStreamer. It uses “recipe” files that indicate how to build
particular projects, and on what other projects they depend.
Moreover, the built projects can be combined into packages for
distribution. These packages are, depending on the target platform,
Windows or OS X installers or Linux packages.

To use Cerbero to build and package your application, you just need to
add a recipe explaining how to build you application and make it depend
on the needed recipes. Then Cerbero can take care of building
your application and its dependencies and package them all together.

Read [](installing/building-from-source-using-cerbero.md) to learn how
to install and use Cerbero.

At this point, after reading the Build from source section in
[](installing/building-from-source-using-cerbero.md), you should be able to
build GStreamer from source and are ready to create recipe and package
files for your application.

In the Cerbero installation directory you will find the
`cerbero-uninstalled` script. Execute it without parameters to see the
list of commands it accepts:

``` bash
./cerbero-uninstalled
```

## Adding a recipe for your application

The first step is to create an empty recipe that you can then tailor to
your needs:

``` bash
./cerbero-uninstalled add-recipe my-app 1.0
```

This will create an initial recipe file in `recipes/my-app.recipe`,
which contains the smallest necessary recipe. This file is a Python
script; set the following attributes to describe your application:

```
+-------------------+-----------------------+----------+----------------------+
| Attribute Name    | Description           | Required | Example              |
|-------------------|-----------------------|----------|----------------------|
| name              | The recipe name       | Yes      | name = 'my-app'      |
|                   |                       |          |                      |
| version           | The software version  | Yes      | version = '1.0'      |
|                   |                       |          |                      |
| licenses          | A list of licenses of | Yes      | licenses =           |
|                   | the software [1]      |          | [License.LGPLv2Plus] |
|                   |                       |          |                      |
| deps              | A list of build       | No       | deps = ['other',     |
|                   | dependencies of the   |          | 'recipe', 'names']   |
|                   | software as recipe    |          |                      |
|                   | names.                |          |                      |
|                   |                       |          |                      |
| platform_deps     | Platform specific     | No       | platform_deps =      |
|                   | build dependencies    |          | {Platform.LINUX:     |
|                   |                       |          | ['some-recipe'],     |
|                   |                       |          | Platform.WINDOWS:    |
|                   |                       |          | ['another-recipe']}  |
|                   |                       |          |                      |
| remotes           | Dictionary specifying | No       | remotes = {'origin': |
|                   | the git remote URLs   |          | '<git://somewhere>'} |
|                   | where sources are     |          |                      |
|                   | pulled from           |          |                      |
|                   |                       |          |                      |
| commit            | The git commit, tag or| No       | commit =             |
|                   | branch to use,        |          | 'my-app-branch'      |
|                   | defaulting to         |          |                      |
|                   | "sdk-*`version`*"     |          |                      |
|                   |                       |          |                      |
| config_sh         | Used to select the    | No       | config_sh =          |
|                   | configuration script  |          | 'autoreconf -fiv &&  |
|                   |                       |          | sh ./configure'      |
|                   |                       |          |                      |
| configure_options | Additional options    | No       | configure_options =  |
|                   | that should be passed |          | '--enable-something' |
|                   | to the `configure`    |          |                      |
|                   | script                |          |                      |
|                   |                       |          |                      |
| use_system_libs   | Whether to use system | No       | use_system_libs =    |
|                   | provided libs         |          | True                 |
|                   |                       |          |                      |
| btype             | The build type        | No       | btype =              |
|                   |                       |          | BuildType.CUSTOM     |
|                   |                       |          |                      |
| stype             | The source type       | No       | stype =              |
|                   |                       |          | SourceType.CUSTOM    |
|                   |                       |          |                      |
| files_category    | A list of files that  | Yes      | files_bins =         |
|                   | should be shipped with|          | ['some-binary']      |
|                   | packages including    |          | files_libs =         |
|                   | this recipe *category*|          | ['libsomelib']       |
|                   | [5]                   |          | files_devel =        |
|                   |                       |          | ['include/something']|
|                   |                       |          | files_python =       |
|                   |                       |          | ['site-packages/     |
|                   |                       |          | some/pythonfile%(pext|
|                   |                       |          | )s']                 |
|                   |                       |          | files_lang = ['foo'] |
|                   |                       |          |                      |
| platform_files_   | Same as files_category| No       | platform_files_some_ |
| category          | but for platform      |          | category =           |
|                   | specific files        |          | {Platform.LINUX:     |
|                   |                       |          | ['/some/file']}      |
+-------------------+-----------------------+----------+----------------------+

[1] See cerbero/enums.py:License for allowed licenses
[2] See cerbero/enums.py:Platform for allowed platforms
[3] See cerbero/build/build.py:BuildType for allowed build types
[4] See cerbero/build/source.py:SourceType for allowed source types
[5] Cerbero comes with some predefined categories that should be used if the
files being installed match a category criteria. The predefined categories are: `libs` (for libraries), `bins` (for binaries), `devel` (for development files:
header, pkgconfig files, etc), `python` (for python files) and `lang` (for
language files). Note that for the `bins` and `libs` categories there is no need
to specify the files extensions as Cerbero will do it for you.
```

> ![warning] At least one “files” category should be set.

Apart from the attributes listed above, it is also possible to override
some Recipe methods. For example the `prepare` method can be overridden
to do anything before the software is built, or the `install` and
`post_install` methods for overriding what should be done during or
after installation. Take a look at the existing recipes in
`cerbero/recipes` for example.

Alternatively, you can pass some options to `cerbero-uninstalled` so some
of these attributes are already set for you. For example:

```
./cerbero-uninstalled add-recipe --licenses "LGPL" --deps "glib" --origin "git://git.my-app.com" --commit "git-commit-to-use" my-app 1.0
```

See `./cerbero-uninstalled add-recipe -h` for help.

As an example, this is the recipe used to build the Pitivi video editor:

```
class Recipe(recipe.Recipe):
    name = 'pitivi'
    version = '0.95'
    licenses = [License.GPLv2Plus]
    remotes = {'origin': 'git://git.gnome.org/pitivi'}
    config_sh = 'sh ./autogen.sh --noconfigure && ./configure'
    configure_options = "--disable-help"
    commit = 'origin/master'
    deps = ['gst-editing-services-1.0',
            'gst-python-1.0',
            'gst-libav-1.0',
            'gst-plugins-bad-1.0',
            'gst-plugins-ugly-1.0',
            'gst-transcoder',
            'numpy',
            'matplotlib',
            'gnome-icon-theme',
            'gnome-icon-theme-symbolic',
            'shared-mime-info'] # brings in gtk+

    files_libs = ['libpitivi-1.0']
    files_typelibs = [
        'Pitivi-1.0',
    ]
    use_system_libs = True
    files_bins = ['pitivi']
    files_lang = ['pitivi']
    files_pitivi = ['lib/pitivi/python/pitivi',
                    'share/pitivi/',
                    'share/applications/pitivi.desktop']
```

Cerbero gets the software sources to build from a GIT repository, which
is specified via the `git_root` configuration variable from the Cerbero
configuration file (see the "Build from software" section in [Installing
on Linux](installing/on-linux.md)) and can be overridden by the
`remotes` attribute inside the recipes (if setting the `origin` remote).
In this case where no “commit” attribute is specified, Cerbero will use
the commit named “sdk-0.2+git” from the GIT repository when building
Snappy.

Once the recipe is ready, instruct Cerbero to build it:

``` bash
./cerbero-uninstalled build my-app
```

## Adding a package for you software

To distribute your software with GStreamer it is necessary to put it into
a package or installer, depending on the target platform. This is done
by selecting the files that should be included. To add a package you
have to create a package file in `cerbero/packages`. The package files
are Python scripts too and there are already many examples of package
files in `cerbero/packages`.

Now, to create an empty package, do:

``` bash
./cerbero-uninstalled add-package my-app 1.0
```

This will create an initial package file in `packages/my-app.package`.

The following Package attributes are used to describe your package:

```
+---------------------+--------------------+----------+------------------------+
| Attribute Name      | Description        | Required | Example                |
|---------------------|--------------------|----------|------------------------|
| name                | The package name   | Yes      | name = 'my-app'        |
|                     |                    |          |                        |
| shortdesc           | A short description| No       | shortdesc = 'some-short|
|                     | of the package     |          | -desc'                 |
|                     |                    |          |                        |
| longdesc            | A long description | No       | longdesc = 'Some Longer|
|                     |                    |          | Description'           |
|                     |                    |          |                        |
| codename            | Release codename.  | No       | codename =             |
|                     |                    |          | 'MyAppReleaseName'     |
|                     |                    |          |                        |
| vendor              | Package Vendor     | No       | vendor = 'MyCompany'   |
|                     |                    |          |                        |
| url                 | Package URL        | No       | url =                  |
|                     |                    |          |'http://www.my-app.com' |
|                     |                    |          |                        |
| version             | Package version    | Yes      | version = '1.0'        |
|                     |                    |          |                        |
| license             | Package license [1]| Yes      | license =              |
|                     |                    |          | License.LGPLv2Plus     |
|                     |                    |          |                        |
| uuid                | Package unique id  | Yes      | uuid =                 |
|                     |                    |          | '6cd161c2-4535-411f-82'|
|                     |                    |          |                        |
| deps                | List of package    | No       | deps = ['other',       |
|                     | dependencies as    |          | 'package', 'names']    |
|                     | package names      |          |                        |
|                     |                    |          |                        |
| sys_deps            | System dependencies| No       | sys_deps=              |
|                     | for this package   |          | {Distro.DEBIAN:        |
|                     |                    |          | ['python']}            |
|                     |                    |          |                        |
| files               | A list of files    | Yes      | files = ['my-app']     |
|                     | included in the    |          | files = ['my-app:      |
|                     | runtime package in |          | category1']            |
|                     | the form           |          |                        |
|                     | "recipe_name:      |          |                        |
|                     | category1:category2|          |                        |
|                     | :..." [2]          |          |                        |
|                     |                    |          |                        |
| files_devel         | A list of files    | Yes      | files_devel =          |
|                     | included in the    |          | ['my-app:category_devel|
|                     | devel package. Same|          | ']                     |
|                     | format as above    |          |                        |
|                     |                    |          |                        |
| platform_files      | Same as *files* but| Yes      | platform_files =       |
|                     | allowing to specify|          | {Platform.WINDOWS:     |
|                     | different files for|          | ['my-app:              |
|                     | different platforms|          | windows_only_category']|
|                     |                    |          | }                      |
|                     |                    |          |                        |
| platform_files_devel| As above but for   | Yes      | platform_files_devel = |
|                     | files_devel        |          | {Platform.WINDOWS:     |
|                     |                    |          | ['my-app:windows_only_c|
|                     |                    |          | ategory_devel']}       |
+---------------------+--------------------+----------+------------------------+

[1] See cerbero/enums.py:License for allowed licenses.
[2] If the recipe category is omitted, all categories are included.

```

> ![warning] At least one of the “files” attributes should be set.

Alternatively you can also pass some options to `cerbero-uninstalled`,
for example:

``` bash
./cerbero-uninstalled add-package my-app 1.0 --license "LGPL" --codename MyApp --vendor MyAppVendor --url "http://www.my-app.com" --files=my-app:bins:libs --files-devel=my-app:devel --platform-files=linux:my-app:linux_specific --platform-files-devel=linux:my-app:linux_specific_devel,windows:my-app:windows_specific_devel --deps base-system --includes gstreamer-core
```

See `./cerbero-uninstalled add-package -h` for help.

As an example, this is the package file that is used for packaging the
`gstreamer-core` package:

```
class Package(package.Package):

    name = 'gstreamer-1.0-codecs'
    shortdesc = 'GStreamer 1.0 codecs'
    longdesc = 'GStreamer 1.0 codecs'
    version = '1.9.0.1'
    url = "http://gstreamer.freedesktop.org"
    license = License.LGPL
    vendor = 'GStreamer Project'
    org = 'org.freedesktop.gstreamer'
    uuid = 'a2e545d5-7819-4636-9e86-3660542f08e5'
    deps = ['gstreamer-1.0-core', 'base-crypto']

    files = ['flac:libs', 'libkate:libs', 'libdv:libs',
            'libogg:libs', 'schroedinger:libs', 'speex:libs',
            'libtheora:libs', 'wavpack:libs', 'libvpx:libs',
            'taglib:libs', 'opus:libs', 'libvorbis:libs',
            'openjpeg:libs', 'openh264:libs', 'spandsp:libs',
            'gst-plugins-base-1.0:plugins_codecs',
            'gst-plugins-good-1.0:plugins_codecs',
            'gst-plugins-bad-1.0:plugins_codecs',
            'gst-plugins-ugly-1.0:plugins_codecs']
    files_devel = ['gst-plugins-base-1.0-static:plugins_codecs_devel',
            'gst-plugins-good-1.0-static:plugins_codecs_devel',
            'gst-plugins-bad-1.0-static:plugins_codecs_devel',
            'gst-plugins-ugly-1.0-static:plugins_codecs_devel',
            'gst-plugins-bad-1.0-static:codecs_devel']
    platform_files = {
            Platform.ANDROID: ['tremor:libs'],
            Platform.IOS: ['tremor:libs']
    }
```

At this point you have two main options: you could either have a single
package that contains everything your software needs, or depend on a
shared version of GStreamer.

### Having a private version of GStreamer

To have a private version of GStreamer included in a single package you
don't have to add the `deps` variable to the package file but instead
list all files you need in the `files` variables. If you decide to go
this road you must make sure that you use a different prefix than
GStreamer in the Cerbero configuration file, otherwise your package
will have file conflicts with GStreamer.

### Having a shared version of GStreamer

If you decide to use a shared version of GStreamer you can create a
package file like the other package files in GStreamer. Just
list all packages you need in the `deps` variable and put the files your
software needs inside the `files` variables. When building a package
this way you must make sure that you use the same prefix and
`packages_prefix` as the ones in your Cerbero configuration file.

Finally, build your package by using:

``` bash
./cerbero-uninstalled package your-package
```

Where `your-package` is the name of the `.package` file that you created
in the `packages` directory. This command will build your software and
all its dependencies, and then make individual packages for them (both
the dependencies and your software). The resulting files will be in the
current working directory.


 [warning]: images/icons/emoticons/warning.svg
