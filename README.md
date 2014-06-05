# CMK - Configuration Management Kit

## Introduction

CMK was created out of the need that features has a lot of functionality which is well tessted and great, but that features has certain automatisms that make it very hard to rely on it. Features often does things that the developer did not intended it to do, which makes it hard to use for the Enterprise.

## Getting started

To get started, the easiest is to check the existing drush commands:

* drush cmk-list
* drush cmk-status

* drush cmk-export

* drush cmk-revert
* drush cmk-update

### Tutorial

1. Enable CMK module and download dependencies
2. Choose a module to export exportables to. It needs to already exist.
3. Use 'drush cmk-list' to list all possible exportables.
4. Use 'drush cmk-export module type exportable' to export the exportable with the given type to the module.
5. Add 'require\_once \_\_DIR\_\_ .  "/module.cmk.inc";' to the modules .module file.

### Common Tasks

- To convert an existing feature module manually, the require\_once should just be added and the features require be removed once all exportables have been exported.
- To convert it automatically, use 'drush cmk-feature-convert feature\_module\_name'

- To update an existing CMK export, 'drush cmk-update module\_name' should be used.

- To revert an existing CMK export, 'drush cmk-revert module\_name' should be used.

### Notes

* Just enabling a module with CMK exports does nothing for things that already exist in the database.
* It is needed to explicitly _revert_ the module or some components of it. (e.g. field\_base, field\_instance)
* This is done to give the developer control over the process of what to revert when.

### Options

* You can add --status=code,db,overridden or --status=c,d,o to filter the list by status, e.g. --status=d.

This is useful when you want to export all non-exported exportables to a module, potentially filtered by type.

## Drush commands

### Introduction

In general every command takes up to three parameters:

- *module* - The module to take action on. This is required for all commands except *cmk-list*.
- *type* - The type of the exportable. This is required when a name is specified.
- *name* - The name of the exportable. This is required for exporting.

This means there is granular control to revert or update any exportable or exportable type or even the whole module.

### Module Status - cmk-status ###

````
$ drush cms falcon_config # Status of falcon_config module - to be removed as its the same as cml falcon_config
````

### Listings - cmk-list ###

````
$ drush cml # List of all exportables in the system.
$ drush cml "" views/view # List and status of all views/view exportables in the system.

$ drush cml falcon_config views/view # List of views within falcon_config module
````

### Exporting - cmk-export ###

````
$ drush cme falcon_config field_group 'group_a|node|article|form'
Are you sure you would like to export group_a|node|article|form to Falcon Config and remove it from the database? (y/n): y
Wrote group_a|node|article|form to sites/all/modules/custom/falcon_config/exports/field_group/field_group/group_a|node|article|form.php.
You may need to clear cache now.
````

#### Multiple exporting

````
$ drush cme falcon_config --status=d field_instance
The following exportables have been found:
Features field_instance -- features/field_instance
     node-article-field_description                     node-article-field_description                       Undefined           
     node-article-field_image                           node-article-field_image                             Undefined
Are you sure you would like to export all selected exportables to Falcon Config and remove them from the database? (y/n): y
Wrote node-article-field_description to sites/all/modules/custom/falcon_config/exports/features/field_instance/node-article-field_description.php.
Wrote node-article-field_image to sites/all/modules/custom/falcon_config/exports/features/field_instance/node-article-field_image.php.
All exportables have been exported and reverted. You may need to clear cache now.
````

### Updating - cmk-update ###

````
$ drush cmu falcon_config 
The following exportables have been found:
Fieldgroup field_group -- field_group/field_group
     group_a|node|article|form                           group_a|node|article|form                           In code             
Features field_instance -- features/field_instance
     node-article-field_description                     node-article-field_description                       Undefined           
     node-article-field_image                           node-article-field_image                             Undefined
Views view -- views/view
     most_recent                                        Most recent articles (most_recent)                   In code
Are you sure you would like to export all displayed exportables to Falcon Config and remove them from the database? (y/n): y
Wrote group_a|node|article|form to sites/all/modules/custom/falcon_config/exports/field_group/field_group/group_a|node|article|form.php.
Wrote node-article-field_description to sites/all/modules/custom/falcon_config/exports/features/field_instance/node-article-field_description.php.
Wrote node-article-field_image to sites/all/modules/custom/falcon_config/exports/features/field_instance/node-article-field_image.php.
Wrote most_recent to sites/all/modules/custom/falcon_config/exports/views/view/most_recent.php.
All exportables have been re-exported and reverted. You may need to clear cache now.
````

#### Updating - with control ####

````
$ drush -y cmu falcon_config field_group # Reverts just the field_group exportables in the module.
# ...
$ drush -y cmu falcon_config field_group group_a|node|article|form # Reverts just the specified exportable in the module.
````

### Reverting - cmk-revert ###

````
$ drush cmr falcon_config field_instance
The following exportables have been found:
Features field_instance -- features/field_instance
     node-article-field_description                   node-article-field_description                   Undefined           
     node-article-field_image              node-article-field_image              Undefined
Are you sure you would like to revert all displayed exportables for Falcon Config and remove them from the database? (y/n): y
All selected exportables have been reverted. You may need to clear cache now.

$ drush cmr falcon_config field_instance node-article-field_description
# ...
````

## API commands

The currently implemented API commands are:

- cmk\_list($module = NULL, $type = NULL, $name = NULL, $filter\_status = FALSE) - Lists all exportables in the system or exports of a module.
- cmk\_revert($module, $type = NULL, array $names = NULL) - Reverts the exports of $module - optionally by type.
- cmk\_load\_exports($module, $type) - Load exports from a module with a given type.
- cmk\_export\_to\_files($module, $type, $name) - Returns a files array with exports for the given module, when adding $name of type $type to it.

### Example usage

In hook\_update\_N():

- cmk\_revert('module', 'views\_view', array('most\_recent'))
 
## File Structure

CMK uses a flat file structure to store all data:

Within a module there is an exports/ directory and a <module>.cmk.inc.

The only thing that needs to be added to a module to enable it for CMK usage is:

````php
require_once '<module>.cmk.inc';
````

### Exportable owning modules

Directly below the exports/ directory can all the modules be found that "own" an exportable:

- features
- views
- page\_manager
- ...

There is also a file called state.php in which different types can be activated or de-activated.

### Exportable types

Below the module level, there can the actual exportable type be found:

- features/image
- features/user\_permission
- features/user\_role
- views/view
- page\_manager/pages

Also in these directories the state of the actual exports is stored in a file called like the type with the extension php:

- features/image.php
- features/user\_permission.php
- features/user\_role.php
- views/view.php
- page\_manager/pages.php

There is usually also a file that contains the default hook, so that it does not need to be specified manually:

- features/image.default\_hook.php
- features/user\_permission.default\_hook.php
- features/user\_role.default\_hook.php
- views/view.default\_hook.php
- page\_manager/pages.default\_hook.php

This is e.g. <module>\_default\_page\_manager\_pages().

### Exportables

Below the modules and exportable types, can the actual exportables be found, e.g.:

- features/image/medium.php
- views/view/most\_recent.php

### Sample Directory structure

This is a sample directory structure:

````
exports
exports/features
exports/features/image
exports/features/image/medium.php
exports/features/image.default_hook.php
exports/features/image.php
exports/features/user_permission
exports/features/user_permission/administer content.php
exports/features/user_permission.default_hook.php
exports/features/user_permission.php
exports/features/user_role
exports/features/user_role/administrator.php
exports/features/user_role.default_hook.php
exports/features/user_role.php
exports/page_manager
exports/page_manager/pages
exports/page_manager/pages/homepage.php
exports/page_manager/pages.default_hook.php
exports/page_manager/pages.php
exports/state.php
````
### Modifying files directly. 

CMK supports to manually edit the files directly. For example by editing exports/page\_manager/pages.php a new page can easily be exported.

Just run:

````
$ drush cmu <module> page\_manager/pages
````

after the changes to export the newly added exportables to the code.

While the drush command for exporting is pretty limited currently, using this strategy, many assets can be exported at once.

## File Contents - Export structure

### Exportable - e.g. views/view/most\_recent.php

````php
$view = new view();
// ...
$cmk_export['most_recent'] = $view;
````

### Export type - e.g. views/view.php and views/view.default\_hook.php

````php
$cmk_exports = array();
$cmk_exports['views_view']['most_recent'] = TRUE; // TRUE = enabled, FALSE = disabled
$cmk_exports['views_view']['most_recent_old'] = FALSE;
````

Using this an alternative version of the most\_recent view can be shipped, but it won't be visible in the system.

The default hook looks for example like:

````php
/**
 * Implements hook_image_default_styles().
 */
function module_views_default_views() {
  return cmk_load_exports('<module>', 'views_view');
}
````

It is very important that cmk\_load\_exports is used inside the hook default functions to give CMK control of which exports to load and which to hide. 

### Module State - state.php

````php
$cmk_export_types['views_view'] = TRUE;
````

This defines all export types and their state. A whole exportable can be disabled here temporarily. This can be useful for example for an upgrade path, where a certain exportable type should just be active after a set of DB operations.

### Module cmk.inc

This includes all the default hooks via require\_once and also the hook\_ctools\_api() or views\_default\_api().

## CMK Goals

CMK has the following goals:

- Focus on the developer experience first.
 - Give the developer full control of what is happening.
 - Provide an easy to use API.
 - Never do anything automatically.
 - Ensure that everything done via drush can also be done in update hooks.
- Allow fine granular control.
 - If just one field\_instance is changed, then just revert that one field\_instance.
 - Allow fine granular disabling and enabling of exportables.
- Re-use existing functionality.
 - Re-use all functionality of features for importing and exporting - without all its burden and complexity.
 - Directly use ctools where supported. Use features just as a fallback. Provide multiple providers.
