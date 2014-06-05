<?php

//------------------------------------------------------------------------
// CMK Hooks - invoke all

/**
 * Implements hook_cmk_list() on behalf of ctools.
 */
function ctools_cmk_list() {
  $exportables = array();

  ctools_include('export');
  $schemas = ctools_export_get_schemas(TRUE);

  foreach ($schemas as $table => $schema) {
    if (!empty($schema['export']['list callback']) && function_exists($schema['export']['list callback'])) {
      $exportables[$table] = $schema['export']['list callback']();
    }
    else {
      $exportables[$table] = ctools_export_default_list($table, $schema);
    }
    natcasesort($exportables[$table]);
  }

  return $exportables;
}

//------------------------------------------------------------------------
// CMK Hooks - delegated to an owner.

/**
 * Implements hook_cmk_type_info() on behalf of ctools.
 */
function ctools_cmk_type_info($type) {
  $info = ctools_export_get_schema($type);

  return $info;
}

/**
 * Implements hook_cmk_export_status() on behalf of ctools.
 */
function ctools_cmk_export_status($type, $names) {
  $export_states = array();
  foreach ($names as $name) {
    $object = ctools_export_crud_load($type, $name);
    if (!empty($object->type)) {
      $export_states[$name]['status'] = $object->type;
      if (!empty($object->export_module)) {
        $export_states[$name]['module'] = $object->export_module;
      }
    }
  }

  return $export_states;
}

/**
 * Implements hook_cmk_export_to_code on behalf of ctools.
 */
function ctools_cmk_export_to_code($module, $type, $name) {

  $schema = ctools_export_get_schema($type);
  $identifier = $schema['export']['identifier'];
  $default_hook = $schema['export']['default hook'];

  $object = ctools_export_crud_load($type, $name);
  // @todo error handling

  // Page manager pages export is actually slightly broken. :(
  if ($type == 'page_manager_pages') {
    page_manager_get_task('page');
    $output = page_manager_page_export($object, TRUE);
  }
  else {
    $output = ctools_export_crud_export($type, $object);
  }
  $output .= "\n";
  $output .= "\$cmk_export['" . $name . "'] = \${$identifier};\n";

  $code = array(
    $default_hook => $output,
  );

  return $code;
}

/**
 * Implements hook_cmk_revert on behalf of ctools.
 */
function ctools_cmk_revert($module, $type, $names) {
  foreach ($names as $name) {
    $object = ctools_export_crud_load($type, $name);
    if ($object && ($object->export_type & EXPORT_IN_DATABASE)) {
      ctools_export_crud_delete($type, $object);
    }
  }
}
