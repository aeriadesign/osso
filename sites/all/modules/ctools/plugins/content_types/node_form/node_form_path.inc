<?php
// $Id: node_form_path.inc,v 1.1 2009/04/18 02:00:35 merlinofchaos Exp $

/**
 * Implementation of specially named hook_ctools_content_types()
 */
function ctools_node_form_path_ctools_content_types() {
  if (module_exists('path')) {
    return array(
      'single' => TRUE,
      'icon' => 'icon_node_form.png',
      'title' => t('Node form url path settings'),
      'description' => t('Publishing options on the Node form.'),
      'required context' => new ctools_context_required(t('Form'), 'node_form'),
      'category' => t('Form'),
    );
  }
}

function ctools_node_form_path_content_type_render($subtype, $conf, $panel_args, &$context) {
  $block = new stdClass();
  $block->module = t('node_form');

  $block->title = t('URL path options');
  $block->delta = 'url-path-options';

  if (isset($context->form)) {
    if (!empty($context->form->form_id) && !empty($context->form['path']['#access'])) {
      // remove the fieldset
      unset($context->form['path']['#type']);
      $context->form['path']['path']['#size'] /= 2;
      $block->content = drupal_render($context->form['path']);
    }
  }
  else {
    $block->content = t('URL Path options.');
  }
  return $block;
}

function ctools_node_form_path_content_type_admin_title($subtype, $conf, $context) {
  return t('"@s" node form path options', array('@s' => $context->identifier));
}

function ctools_node_form_path_content_type_edit_form(&$form, &$form_state) {
  // provide a blank form so we have a place to have context setting.
}
