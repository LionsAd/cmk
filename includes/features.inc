<?php

//------------------------------------------------------------------------
// CMK Hooks - invoke all

/**
 * Implements hook_cmk_list() on behalf of features.
 */
function features_cmk_list() {

  // Load features export functionality.
  module_load_include('inc', 'features', 'features.export');
  features_include();

  $components = array();

  foreach (features_get_feature_components() as $source => $info) {
    if ($options = features_invoke($source, 'features_export_options')) {
      foreach ($options as $name => $title) {
        $components[$source][$name] = $title;
      }
    }
  }

  return $components;
}

//------------------------------------------------------------------------
// CMK Hooks - delegated to an owner.

/**
 * Implements hook_cmk_type_info() on behalf of features.
 */
function features_cmk_type_info($type) {
  $info = features_get_components($type);
  if (!isset($info['module'])) {
    $info['module'] = 'features';
  }

  return $info;
}

/**
 * Implements hook_cmk_export_status() on behalf of features.
 */
function features_cmk_export_status($type, $names) {
  // @todo Stub!
  return array();
}

/**
 * Implements hook_cmk_export_to_code on behalf of features.
 */
function features_cmk_export_to_code($type, $name) {

  global $features_ignore_conflicts;
  $features_ignore_conflicts = TRUE;

  module_load_include('inc', 'features', 'features.export');
  features_include();

  // Features needs a module_name, but we just use 'cmk' as a catch-all here.
  $module_name = 'cmk';

  $data = array(
    $name,
  );

  $export = array();
  $export_function = features_hook($type, 'features_export');

  // @todo Support recursive pipe?
  $pipe = $export_function($data, $export, $module_name, $type);

  $code = features_export_render_hooks($export, $module_name);
  if (!isset($code[$type])) {
    return;
  }

  // Convert the types to the format expected by CMK, i.e. the code is in {$type}s.
  foreach ($code[$type] as $default_hook => $inner_code) {
    $code[$type][$default_hook] = _features_cmk_add_function_scope($type, $inner_code);
  }

  return $code[$type];
}

/**
 * Add an inline function to retrieve the result of the features code reliably.
 */
function _features_cmk_add_function_scope($type, $inner_code) {

  // @todo Some default hooks need additional arguments.

  // Features depends on using a function definition, so
  // we convert it in an inline function instead.

  $code = <<<EOF
\${$type}s_function = function () {
$inner_code
};
\$cmk_export = \${$type}s_function();

EOF;

  return $code;
}

/**
 * Implements hook_cmk_revert on behalf of features.
 */
function features_cmk_revert($module, $type, $names) {
  // Now it gets tricky: We want to restrict the exports
  // to those with the name of component.

  // So we need to temporarily restrict the loading of
  // exports via CMK.
  cmk_restrict_exports($type, $names);

  // Reset features cache.
  features_get_default($type, $module, TRUE, TRUE);

  if (features_hook($type, 'features_revert')) {
    features_invoke($type, 'features_revert', $module);
  }

  // Allow all exports again.
  cmk_allow_exports($type);

  // Reset features cache again just to get back into a clean slate.
  features_get_default($type, $module, TRUE, TRUE);
}
