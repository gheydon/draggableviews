Draggableviews
---------------
This module provides a style plugin for views. This plugin allows dragging nodes and saving the new structure.

Quick install:
 1) Activate Draggableviews module at yoursite?q=admin/build/modules.
 2) Navigate to views edit-page and Click the "+" at the "Fields" section and
    choose "Draggableviews: Order" -> Click Add button.
 3) Click the currently enabled style plugin ("Basic settings" section).
    Choose Draggable Table and click Update button.
 4) Enter style plugin settings by clicking the little cogwheel next to "Draggable Table" style plugin
 5) Set the Order Field from 2) and choose "Native" handler.
    Click Update button.
 6) Save the view and you're done.

Troubleshooting Drag n' drop Not Showing
========================================
1. Make sure javascript is turned on and loading property.  Doublecheck your source code.  For tables (D6) its <root>/misc/tabledrag.js.
2. Make sure you have draggableviews permission for the correct role.
3. Select 'show row weights'.  By default, this is located at the top right of the table. See http://drupal.org/files/draggableviews-1978526-hode-row-weights.png" for a visual image.
4. 'Show row weights' is a global variable/setting.  If you turn it off for 1 table, then all tables, across all pages, across all users, will not see it.  To fix this in the UI, you have to 'hide row weights' on another page/table, such as admin/structure/block (D7) or admin/build/block (D6), or go into the variables table in the database.
