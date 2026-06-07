# Qt Tool Documentation Front Page

The `tool-name.qdoc` file is a template for creating a front page for the
documentation of a Qt tool, such as Qt Creator or Qt Design Studio.

The purpose of the template is to make it easier for technical writers to create
documentation with a consistent look and feel. It was designed by the Qt UX team.
For this to work, you cannot move the sections around. You can and must change
the text within the angle brackets and check the links.

## To use the template

1. Copy the file to the `doc/src` folder in your documentation project.
1. Copy the images that you need from the `images` folder to the `images` folder
   in your documentation project.
1. Change the text within angle brackets to fit your tool.

## Top level table

The top level table should always have the same three columns:

- INSTALLATION
- GETTING STARTED
- TUTORIALS

For best-looking results, try to keep the length of the text in each cell about
the same.

If you don't have tutorials, you can list some other type of topics here, such as
REFERENCE topics.

## ALL TOPICS section

The name and contents of this section depend on your project. For a small doc
set, you can list all topics. For a big doc set, you can group topics.

In the [Qt Creator Documentation](https://doc.qt.io/qtcreator/index.html), this
section is called `HOW TO`.
