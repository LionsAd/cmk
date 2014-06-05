<?php

/**
 * @file
 * CMK Drush commands
 */

/**
 * Implements hook_drush_command().
 */
function cmk_drush_command() {
  $items = array();

  $options = array(
    'status' => 'Comma separated list of statuses to filter by (e.g. code,db,overridden) or the short names c,d,o',
  );

  $items['cmk-status'] = array(
    'aliases' => array('cms'),
    'callback' => 'cmk_drush_status',
    'description' => 'Get a status report of at exports for a module or all modules.',
    'arguments' => array(
      'module'    => 'Name of your module.',
    ),
    'options' => array(),
  );

  $items['cmk-list'] = array(
    'aliases' => array('cml'),
    'callback' => 'cmk_drush_list',
    'description' => 'Get a list of all exportable items or exports of a module.',
    'arguments' => array(
      'module'    => 'Name of your module can be empty.',
      'type'      => 'Type of export, eg "views" or "page_manager/handlers"',
      'name'      => 'Name of the export.'
    ),
    'options' => $options,
  );

  $items['cmk-export'] = array(
    'aliases' => array('cme'),
    'callback' => 'cmk_drush_export',
    'description' => 'Add an exportable item to a module\'s export list.',
    'arguments' => array(
      'module'    => 'Name of your module.',
      'type'      => 'Type of export, eg "views/view" or "page_manager/handlers"',
      'name'      => 'Name of the export.'
    ),
    'options' => $options,
  );

  $items['cmk-update'] = array(
    'aliases' => array('cmu'),
    'callback' => 'cmk_drush_update',
    'description' => 'Update all overridden exportables into code for the module and remove them from the database.',
    'arguments' => array(
      'module'    => 'Name of your module.',
      'type'      => 'Type of export, eg "views" or "page_manager/handlers"',
      'name'      => 'Name of the export.'
    ),
    'options' => $options,
  );

  $items['cmk-revert'] = array(
    'aliases' => array('cmr'),
    'callback' => 'cmk_drush_revert',
    'description' => 'Revert all exportables from the current directory.',
    'arguments' => array(
      'module'    => 'Name of your module.',
      'type'      => 'Type of export, eg "views" or "page_manager/handlers"',
      'name'      => 'Name of the export.'
    ),
    'options' => $options,
  );

  $items['cmk-convert-feature'] = array(
    'aliases' => array('cmcf'),
    'callback' => 'cmk_drush_convert_feature',
    'description' => 'Converts a feature to CMK',
    'arguments' => array(
      'module'    => 'Name of feature module to convert.',
    ),
    'options' => array(),
  );

  $items['cmk-remove'] = array(
    'aliases' => array('cmrm'),
    'callback' => 'cmk_drush_remove',
    'description' => 'Remove selected exportables from code.',
    'arguments' => array(
      'module'    => 'Name of your module.',
      'type'      => 'Type of export, eg "views" or "page_manager/handlers"',
      'name'      => 'Name of the export.'
    ),
    'options' => $options,
  );


  return $items;
}

// -----------------------------------------------------------------------
// Drush specific API functions

/**
 * Returns the human readable name for a given module name.
 *
 * @param string $module
 *   The name of the module.
 *
 * @return string
 *   The human readable name of the module if enabled or FALSE otherwise.
 */
function cmk_drush_get_module_name($module) {
  $modules = system_list('module_enabled');
  if (empty($modules[$module])) {
    return FALSE;
  }

  return $modules[$module]->info['name'];
}

/**
 * Get the printable name for a namespace.
 *
 * For example, page_manager/pages turns into "Page manager pages".
 *
 * @param $namespace
 *   The namespace to parse.
 *
 * @return null|string
 *   The namespace title.
 */
function cmk_drush_get_namespace_title($namespace) {
  if (strpos($namespace, '/') === FALSE) {
    $module = $namespace;
    $group = '';
  }
  else {
    list($module, $group) = explode('/', $namespace);
  }

  $module_name = cmk_drush_get_module_name($module);
  if (empty($module_name)) {
    return t('Unknown module');
  }
  $output = $module_name;

  if ($group) {
    $output .= ' ' . $group;
  }

  return $output;
}

/**
 * Print the status of every exportable in the list.
 *
 * @param array $list
 *   A list of exportables as returned by cmk_list().
 */
function cmk_drush_print_exportables_list($list) {
  $reverse_namespaces = cmk_reverse_namespaces();
  $all_exports = cmk_get_all_exports();

  foreach ($list as $type => $exports) {
    if (empty($exports)) {
      continue;
    }

    drush_print(cmk_drush_get_namespace_title($reverse_namespaces[$type]) . ' -- ' . $reverse_namespaces[$type]);
    $rows = array();
    $export_statuses = cmk_export_status($type, array_keys($exports));

    foreach ($exports as $name => $title) {
      $export_status = isset($export_statuses[$name]) ?
        $export_statuses[$name] :
        array(
          'status' => t('Undefined'),
        );

      if ($export_status['status'] == dt('Normal')) {
        $status = shellColours::getColouredOutput(dt('In database'), 'blue');
      }
      elseif ($export_status['status'] == dt('Default')) {
        $status = shellColours::getColouredOutput(dt('In code'), 'light_green');
      }
      else {
        $status = shellColours::getColouredOutput($export_status['status'], 'red');
      }

      $module = '';
      if (!empty($export_status['module'])) {
        $module = $export_status['module'];
      }
      if (!empty($all_exports[$type][$name])) {
        $module = shellColours::getColouredOutput(implode(', ', $all_exports[$type][$name]), 'green');
        if (count($all_exports[$type][$name]) > 1) {
          $module .= shellColours::getColouredOutput(' -- Double Export !!!', 'red');
        }
      }

      $row = array();
      $row[] = '  ';
      $row[] = $name;
      $row[] = $title;
      $row[] = $status;
      $row[] = $module;
      $rows[] = $row;
    }
    drush_print_table($rows, FALSE, array(2, 50, 50, 30, 40));
  }
}

/**
 * Export an exportable to a file.
 *
 * @param string $module
 *   The module to export to.
 * @param string $type
 *   The type of the exportable to export.
 * @param string $name
 *   The name of the exportable to export.
 */
function cmk_drush_export_to_file($module, $type, $name) {
  $files = cmk_export_to_files($module, $type, $name);

  $path = current(array_keys($files));
  foreach ($files as $filename => $output) {
    file_put_contents($filename, $output);
  }

  drush_print(dt("Wrote @name to @path.", array('@name' => $name, '@path' => $path)));
}

/**
 * Filters a list of exportables by status drush option.
 *
 * @param $list
 *   The list of exports to filter.
 */
function cmk_drush_filter_by_status(&$list) {
  $status_to_check = drush_get_option('status');
  if (empty($status_to_check)) {
    return;
  }
  $status_to_check = str_replace(array('code','db','overridden'), array('c','d','o'), $status_to_check);
  $status_to_check = explode(',', $status_to_check);

  foreach ($list as $type => $names) {
    if (empty($names)) {
      continue;
    }

    $export_statuses = cmk_export_status($type, array_keys($names));

    foreach ($names as $name => $title) {
      $export_status = isset($export_statuses[$name]) ?
        $export_statuses[$name] :
        array(
          'status' => t('Undefined'),
        );

      if ($export_status['status'] == dt('Normal')) {
        $status = 'd';
      }
      elseif ($export_status['status'] == dt('Default')) {
        $status = 'c';
      }
      else {
        $status = 'o';
      }
      if (!in_array($status, $status_to_check)) {
        unset($list[$type][$name]);
      }
    }
    if (empty($list[$type])) {
      unset($list[$type]);
    }
  }
}

// -----------------------------------------------------------------------
// Drush command callbacks.

/**
 * Print the export status for a module.
 *
 * drush cms
 *
 * @param null $module
 *   The module to get status about.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function cmk_drush_status($module = NULL) {
  if (!$module) {
    drush_log(dt('Usage: drush cms MODULE'), 'error');
    return FALSE;
  }

  if (!module_exists($module)) {
    drush_log(dt('Invalid or disabled module.'), 'error');
    return FALSE;
  }

  $module_name = cmk_drush_get_module_name($module);
  drush_print(dt('Status of all CMK handled exports for @module:', array('@module' => $module_name)));

  $exportables = cmk_list($module);
  cmk_drush_print_exportables_list($exportables);

  return TRUE;
}

/**
 * Print the status of all exportables in the system.
 *
 * @param null $module
 *   The optional module to get status about.
 * @param null $type_or_ns
 *   The type or namespace to filter the list by.
 * @param null $name
 *   The name of an exportable to filter the list by.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function cmk_drush_list($module = NULL, $type_or_ns = NULL, $name = NULL) {
  $type = cmk_find_type($type_or_ns);

  if ($type === FALSE && !empty($type_or_ns)) {
    drush_log(dt('Invalid type specified.'), 'error');
    return FALSE;
  }

  drush_print(dt('Status of queried CMK exportables:'));
  $exportables = cmk_list($module, $type, $name);
  cmk_drush_filter_by_status($exportables);
  cmk_drush_print_exportables_list($exportables);

  return TRUE;
}

/**
 * Export the given exportable by name into the module.
 *
 * Example: drush cme mymodule page_manager/pages front_page
 *
 * @param null $module
 *   The module to export to.
 * @param null $type_or_ns
 *   The type or namespace of the exportable.
 * @param null $name
 *   The name of the exportable.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function cmk_drush_export($module = NULL, $type_or_ns = NULL, $name = NULL) {
  if (!$module) {
    drush_log(dt('Usage: drush cme --status=[code,db,overridden] module type [name]'), 'error');
    return FALSE;
  }
  $type = cmk_find_type($type_or_ns);

  if (empty($type)) {
    drush_log(dt('Unable to find export type.'), 'error');
    return FALSE;
  }

  // @todo allow creation of a module
  if (!module_exists($module)) {
    drush_log(dt('Invalid or disabled module.'), 'error');
    return FALSE;
  }

  $exports = cmk_list(NULL, $type, $name);
  cmk_drush_filter_by_status($exports);

  drush_print(dt('The following exportables have been found:'));
  cmk_drush_print_exportables_list($exports);

  // @todo allow disabled modules.
  $module_name = cmk_drush_get_module_name($module);
  $all = drush_confirm(dt('Are you sure you would like to export all displayed exportables to @module and remove them from the database?', array('@module' => $module_name)));
  if (!$all) {
    drush_log(dt('Command cancelled'), 'success');
    return FALSE;
  }

  // @todo Deduplicate this code with cmk-update
  foreach ($exports as $type => $names) {
    if (empty($names)) {
      continue;
    }
    // This is slow, but works well.
    foreach ($names as $name => $description) {
      cmk_drush_export_to_file($module, $type, $name);
    }
    // @todo Use something different here that just removes from the DB?
    cmk_revert($module, $type, array_keys($names));
  }

  drush_print(dt('All exportables have been exported and reverted. You may need to clear cache now.'));
  return TRUE;
}

/**
 * Check every exportable in the module to see if it is overridden and write to code if it is.
 *
 * @param null $module
 *   The module to update.
 * @param null $type_or_ns
 *   The type or namespace to filter the list by.
 * @param null $name
 *   The name of an exportable to filter the list by.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function cmk_drush_update($module = NULL, $type_or_ns = NULL, $name = NULL) {
  $type = cmk_find_type($type_or_ns);

  if ($type === FALSE && !empty($type_or_ns)) {
    drush_log(dt('Invalid type specified.'), 'error');
    return FALSE;
  }

  if (!$module) {
    drush_log(dt('Usage: drush cmu <module> [type] [name]'), 'error');
    return FALSE;
  }

  if (!module_exists($module)) {
    drush_log(dt('Invalid or disabled module.'), 'error');
    return FALSE;
  }

  $module_name = cmk_drush_get_module_name($module);
  $exports = cmk_list($module, $type, $name, FALSE);
  cmk_drush_filter_by_status($exports);

  drush_print(dt('The following exportables have been found:'));
  cmk_drush_print_exportables_list($exports);

  $all = drush_confirm(dt('Are you sure you would like to export all displayed exportables to @module and remove them from the database?', array('@module' => $module_name)));
  if (!$all) {
    drush_log(dt('Command cancelled'), 'success');
    return FALSE;
  }

  foreach ($exports as $type => $names) {
    if (empty($names)) {
      continue;
    }

    foreach ($names as $name => $title) {
      cmk_drush_export_to_file($module, $type, $name);
    }
    // @todo Use something different here that just removes from the DB?
    cmk_revert($module, $type, array_keys($names));
  }
  drush_print(dt('All exportables have been re-exported and reverted. You may need to clear cache now.'));
}

/**
 * Check every exportable in the module and revert back to the code version.
 *
 * @param null $module
 *   The module to revert.
 * @param null $type_or_ns
 *   The type or namespace to filter the list by.
 * @param null $name
 *   The name of an exportable to filter the list by.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function cmk_drush_revert($module = NULL, $type_or_ns = NULL, $name = NULL) {
  $type = cmk_find_type($type_or_ns);

  if ($type === FALSE && !empty($type_or_ns)) {
    drush_log(dt('Invalid type specified.'), 'error');
    return FALSE;
  }

  if (!$module) {
    drush_log(dt('Usage: drush cmr <module> [type] [name]'), 'error');
    return FALSE;
  }

  if (!module_exists($module)) {
    drush_log(dt('Invalid or disabled module.'), 'error');
    return FALSE;
  }

  $module_name = cmk_drush_get_module_name($module);
  $exports = cmk_list($module, $type, $name, TRUE);
  cmk_drush_filter_by_status($exports);

  drush_print(dt('The following exportables have been found:'));
  cmk_drush_print_exportables_list($exports);

  $all = drush_confirm(dt('Are you sure you would like to revert all displayed exportables for @module and remove them from the database?', array('@module' => $module_name)));
  if (!$all) {
    drush_log(dt('Command cancelled'), 'success');
    return FALSE;
  }

  cmk_revert($module, $type, $name);

  drush_print(dt('All selected exportables have been reverted. You may need to clear cache now.'));
}

/**
 * Convert a feature to CMK. 
 *
 * @param string $module
 *   The feature module to convert.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function cmk_drush_convert_feature($module = NULL) {
  if (!$module) {
    drush_log(dt('Usage: drush cmcf <module>'), 'error');
    return FALSE;
  }

  if (!module_exists($module)) {
    drush_log(dt('Invalid or disabled module.'), 'error');
    return FALSE;
  }

  $module_name = cmk_drush_get_module_name($module);
  $exportables = cmk_list();

  features_include();
  module_load_include('inc', 'features', 'features.export');

  $feature = features_get_features($module, TRUE);

  foreach ($feature->info['features'] as $type => $names ) {
    // Ensure we can export this type and its not a pseudo ctools or api type.
    if (!isset($exportables[$type])) {
      continue;
    }
    foreach ($names as $name) {
      $exports[$type][$name] = !empty($exportables[$type][$name]) ? $exportables[$type][$name] : $name;
    }
  }

  drush_print(dt('The following exportables have been found:'));
  cmk_drush_print_exportables_list($exports);

  $all = drush_confirm(dt('Are you sure you would like to convert all displayed exportables for @module to CMK and remove the features from it?', array('@module' => $module_name)));
  if (!$all) {
    drush_log(dt('Command cancelled'), 'success');
    return FALSE;
  }

  foreach ($exports as $type => $names) {
    if (empty($names)) {
      continue;
    }
    // This is slow, but works well.
    foreach ($names as $name => $description) {
      cmk_drush_export_to_file($module, $type, $name);
    }
  }

  // @todo un-featurize the module
  $directory = dirname($feature->uri); 
  $info = $feature->info;

  // First remove all export files.
  $export = features_populate($info, $module);
  $files = features_export_render($export, $module, TRUE);
  foreach ($files as $extension => $file_contents) {
    if (in_array($extension, array('module', 'info'))) {
      continue;
    }
    $extension .= '.inc';
    $filename = "{$directory}/{$module}.$extension";
    if (file_exists($filename)) {
      unlink($filename);
      drush_print(dt('Removed @filename from module.', array('@filename' => $filename)));
    }
  }

  // Then cleanup the exports themselves.

  // Remove all exports from the feature.
  $info['features'] = array();
  $info['features_exclude'] = array();

  $export = features_populate($info, $module);
  $files = features_export_render($export, $module, TRUE);


  // Remove features from .info file.
  $files['info'] = preg_replace("/^package = .*$/m", 'package = CMK', $files['info']);
  $files['info'] = preg_replace("/^features.*$(?:\r\n|\n)?/m", '', $files['info']);
 
  // Require CMK instead of Features include file. 
  $files['module'] = preg_replace("/^include_once '$module.features.inc';$(?:\r\n|\n)?/m", "require_once __DIR__ . '/$module.cmk.inc';", $files['module']);
  
  if (strstr($files['module'], "<?php") === FALSE) {
    $files['module'] = "<?php\n";
  }

  if (strstr($files['module'], "require_once __DIR__ . '/$module.cmk.inc';") === FALSE) {
    $files['module'] .= "\n";
    $files['module'] .= "// Load CMK exports." . "\n";
    $files['module'] .= "require_once __DIR__ '/$module.cmk.inc';" . "\n";
  }

  foreach ($files as $extension => $file_contents) {
    if (!in_array($extension, array('module', 'info'))) {
      continue;
    }
    $filename = "{$directory}/{$module}.$extension";
    file_put_contents($filename, $file_contents);
    drush_print(dt('Converted @filename to CMK.', array('@filename' => $filename)));
  }

  drush_print(dt('All exportables for @module have been converted. You may need to clear cache now.', array('@module' => $module_name)));
}

/**
 * Remove selected exportables from a module.
 *
 * @param string $module
 *   The module to remove the exportables from.
 * @param string $type_or_ns
 *   The type or namespace to filter the list by.
 * @param null $name
 *   The name of an exportable to filter the list by.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function cmk_drush_remove($module = NULL, $type_or_ns = NULL, $name = NULL) {
  $type = cmk_find_type($type_or_ns);

  if ($type === FALSE && !empty($type_or_ns)) {
    drush_log(dt('Invalid type specified.'), 'error');
    return FALSE;
  }

  if (!$module) {
    drush_log(dt('Usage: drush cmrm <module> <type> [name]'), 'error');
    return FALSE;
  }

  if (!module_exists($module)) {
    drush_log(dt('Invalid or disabled module.'), 'error');
    return FALSE;
  }

  $module_name = cmk_drush_get_module_name($module);
  $exports = cmk_list($module, $type, $name);
  cmk_drush_filter_by_status($exports);

  drush_print(dt('The following exportables have been found:'));
  cmk_drush_print_exportables_list($exports);

  $all = drush_confirm(dt('Are you sure you would like to remove all displayed exportables from @module?', array('@module' => $module_name)));
  if (!$all) {
    drush_log(dt('Command cancelled'), 'success');
    return FALSE;
  }

  $files = cmk_remove_from_files($module, $type, $name);

  foreach ($files as $filename => $output) {
    if (!$output && file_exists($filename)) {
      unlink($filename);
      drush_print(dt('Removed @filename from module.', array('@filename' => $filename)));
    } else {
      file_put_contents($filename, $output);
    }
  }

  drush_print(dt('All selected exportables have been removed. You may need to clear cache now.'));
}
