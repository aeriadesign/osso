<?php
// $Id: FeedsUserProcessor.inc,v 1.6 2010/02/17 22:58:18 alexb Exp $

/**
 * @file
 * FeedsUserProcessor class.
 */

/**
 * Feeds processor plugin. Create users from feed items.
 */
class FeedsUserProcessor extends FeedsProcessor {

  /**
   * Implementation of FeedsProcessor::process().
   */
  public function process(FeedsImportBatch $batch, FeedsSource $source) {

    // Count number of created and updated nodes.
    $created  = $updated = $failed = 0;

    while ($item = $batch->shiftItem()) {

      if (!($uid = $this->existingItemId($item, $source)) || $this->config['update_existing']) {

        // Map item to a term.
        $account = $this->map($item);

        // Check if user name and mail are set, otherwise continue.
        if (empty($account->name) || empty($account->mail) || !valid_email_address($account->mail)) {
          $failed++;
          continue;
        }

        // Add term id if available.
        if (!empty($uid)) {
          $account->uid = $uid;
        }

        // Save the user.
        user_save($account, (array) $account);
        if ($account->uid && $account->openid) {
          $authmap = array(
            'uid' => $account->uid,
            'module' => 'openid',
            'authname' => $account->openid,
          );
          if (SAVED_UPDATED != drupal_write_record('authmap', $authmap, array('uid', 'module'))) {
            drupal_write_record('authmap', $authmap);
          }
        }

        if ($uid) {
          $updated++;
        }
        else {
          $created++;
        }
      }
    }

    // Set messages.
    if ($failed) {
      drupal_set_message(t('There were !number users that could not be imported because either their name or their email was empty or not valid. Check import data and mapping settings on User processor.', array('!number' => $failed)), 'error');
    }
    if ($created) {
      drupal_set_message(t('Created !number users.', array('!number' => $created)));
    }
    elseif ($updated) {
      drupal_set_message(t('Updated !number users.', array('!number' => $updated)));
    }
    else {
      drupal_set_message(t('There are no new users.'));
    }

    return FEEDS_BATCH_COMPLETE;
  }

  /**
   * Implementation of FeedsProcessor::clear().
   */
  public function clear(FeedsBatch $batch, FeedsSource $source) {
    // Do not support deleting users as we have no way of knowing which ones we
    // imported.
    throw new Exception(t('User processor does not support deleting users.'));
  }

  /**
   * Execute mapping on an item.
   */
  protected function map($source_item) {
    // Prepare term object.
    $target_account = new stdClass();
    $target_account->uid = 0;
    $target_account->roles = $this->config['roles'];
    $target_account->status = $this->config['status'];

    // Have parent class do the iterating.
    return parent::map($source_item, $target_account);
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array(
      'roles' => array(),
      'update_existing' => FALSE,
      'status' => 1,
      'mappings' => array(),
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = array();

    $form['status'] = array(
      '#type' => 'radios',
      '#title' => t('Status'),
      '#description' => t('Select whether users should be imported active or blocked.'),
      '#options' => array(0 => t('Blocked'), 1 => t('Active')),
      '#default_value' => $this->config['status'],
    );

    $roles = user_roles(TRUE);
    unset($roles[2]);
    if (count($roles)) {
      $form['roles'] = array(
        '#type' => 'checkboxes',
        '#title' => t('Additional roles'),
        '#description' => t('Every user will be assigned the "autenticated user" role. Select additional roles here.'),
        '#default_value' => $this->config['roles'],
        '#options' => $roles,
      );
    }

    $form['update_existing'] = array(
      '#type' => 'checkbox',
      '#title' => t('Update existing users'),
      '#description' => t('Check if existing users should be updated from imported data.'),
      '#default_value' => $this->config['update_existing'],
    );
    return $form;
  }

  /**
   * Set target element.
   */
  public function setTargetElement(&$target_item, $target_element, $value) {
    $target_item->$target_element = $value;
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    $targets = array(
      'name' => array(
        'name' => t('User name'),
        'description' => t('Name of the user.'),
        'optional_unique' => TRUE,
       ),
      'mail' => array(
        'name' => t('Email address'),
        'description' => t('Email address of the user.'),
        'optional_unique' => TRUE,
       ),
      'created' => array(
        'name' => t('Created date'),
        'description' => t('The created (e. g. joined) data of the user.'),
       ),
    );
    if (module_exists('openid')) {
      $targets['openid'] = array(
        'name' => t('OpenID identifier'),
        'description' => t('The OpenID identifier of the user. <strong>CAUTION:</strong> Use only for migration purposes, misconfiguration of the OpenID identifier can lead to severe security breaches like users gaining access to accounts other than their own.'),
        'optional_unique' => TRUE,
       );
    }
    return $targets;
  }

  /**
   * Get id of an existing feed item term if available.
   */
  protected function existingItemId($source_item, FeedsSource $source) {

    // Iterate through all unique targets and try to find a user for the
    // target's value.
    foreach ($this->uniqueTargets($source_item) as $target => $value) {
      switch ($target) {
        case 'name':
          $uid = db_result(db_query('SELECT uid FROM {users} WHERE name = "%s"', $value));
          break;
        case 'mail':
          $uid = db_result(db_query('SELECT uid FROM {users} WHERE mail = "%s"', $value));
          break;
        case 'openid':
          $uid = db_result(db_query("SELECT uid FROM {authmap} WHERE authname = '%s' AND module = 'openid'", $value));
          break;
      }
      if ($uid) {
        // Return with the first nid found.
        return $uid;
      }
    }
    return 0;
  }
}