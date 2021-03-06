<?php

/**
 * Edit form callback.
 */
function openid_provider_sso_rp_edit_form() {
  if ($_GET['realm']) {
    $rp = openid_provider_sso_relying_party($_GET['realm']);
  }
  $form = array();
  $form['#redirect'] = 'admin/settings/openid-provider-sso';
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => empty($rp) ? '' : $rp['name'],
    '#required' => TRUE,
  );
  if (isset($rp)) {
    $form['realm'] = array(
      '#type' => 'hidden',
      '#default_value' => empty($rp) ? '' : $rp['realm'],
    );
  }
  else {
    $form['realm'] = array(
      '#type' => 'textfield',
      '#title' => t('Realm (URL)'),
      '#default_value' => empty($rp) ? '' : $rp['realm'],
      '#required' => TRUE,
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Submit handler for openid_provider_sso_rp_edit_form().
 */
function openid_provider_sso_rp_edit_form_submit($form, &$form_state) {
  openid_provider_sso_rp_add($form_state['values']['realm'], $form_state['values']['name']);
  drupal_set_message(t('Added Relying Party @realm (@name).', array('@realm' => $form_state['values']['realm'], '@name' => $form_state['values']['name'])));
}

/**
 * Remove form callback.
 */
function openid_provider_sso_rp_remove_form($form_state) {
  if ($rp = openid_provider_sso_relying_party($_GET['realm'])) {
    $form = array();
    $form['#realm'] = $rp['realm'];
    $form['#redirect'] = 'admin/settings/openid-provider-sso';
    $question = t('Remove Relying Party?');
    $description = t('If you remove the Relying Party @realm (@name), it cannot use this OpenID Provider site for authentication anymore. You can add it back at any point later.', array('@realm' => $rp['realm'], '@name' => $rp['name']));
    return confirm_form($form, $question, $form['#redirect'], $description, t('Remove'));
  }
}

/**
 * Remove form submit handler.
 */
function openid_provider_sso_rp_remove_form_submit($form, &$form_state) {
  openid_provider_sso_rp_remove($form['#realm']);
}

/**
 * Admin settings page callback.
 */
function openid_provider_sso_rps_page() {
  $rps = openid_provider_sso_relying_parties();
  foreach ($rps as $rp) {
    $edit = l(t('edit'), 'admin/settings/openid-provider-sso/edit', array('query' => 'realm='. urlencode($rp['realm'])));
    $remove = l(t('remove'), 'admin/settings/openid-provider-sso/remove', array('query' => 'realm='. urlencode($rp['realm'])));
    $rows[] = array(
      l(check_plain($rp['realm']), $rp['realm']),
      check_plain($rp['name']),
      $edit .' | '. $remove,
    );
  }
  return theme_table(array(t('Realm'), t('Name')), $rows);
}
