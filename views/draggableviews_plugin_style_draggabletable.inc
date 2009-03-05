<?php
// $Id$

/**
 * @file
 * Draggableviews style plugin definition.
 */

/**
 * Style plugin to render each item as a row in a draggable table;
 * Inherits all from views_plugin_table.
 */
class draggableviews_plugin_style_draggabletable extends views_plugin_style_table {
  function option_definition() {
    $options = parent::option_definition();

    $options['columns'] = array('default' => array());
    $options['default'] = array('default' => '');
    $options['info'] = array('default' => array());
    $options['override'] = array('default' => TRUE);
    $options['sticky'] = array('default' => FALSE);
    $options['order'] = array('default' => 'asc');

    return $options;
  }

  function validate() {
    $errors = array();

    // check if first field is draggableviews handled and will be hidden
    $found = FALSE;
    $handlers = $this->display->handler->get_handlers('field');
    $first_field = key($handlers);
    if (isset($this->options['tabledrag_order'])) {
      for ($i = 0; $i < count($this->options['tabledrag_order']); $i++) {
        if ($this->options['tabledrag_order'][0]['field'] == $first_field && $this->options['tabledrag_order_visible']['visible'] !== 'visible') $found = TRUE;
      }
    }
    if (isset($this->options['tabledrag_hierarchy'])) {
      for ($i = 0; $i < count($this->options['tabledrag_hierarchy']); $i++) {
        if ($this->options['tabledrag_hierarchy']['field'] == $first_field && $this->options['tabledrag_hierarchy_visible']['visible'] !== 'visible') $found = TRUE;
      }
    }
    if ($found) {
      $errors[] = t('Display "@display": The first field must not be Draggableviews-handled when input fields will not be shown.', array('@display' => $this->display->display_title));
    }

    if (isset($this->options['tabledrag_order'])) {
      // check if all configured fields are available
      foreach ($this->options['tabledrag_order'] as $field) {
        if (!isset($handlers[$field['field']])) {
          $errors[] = t('Display "@display": Draggableviews: order field <i>@field</i> could not be found (..afterwards removed from Fields section?). Check your settings.', array('@display' => $this->display->display_title, '@field' => $field['field']));
        }
      }
    }
    if (isset($this->options['tabledrag_hierarchy'])) {
      if ($this->options['tabledrag_hierarchy']['field'] != 'none' && !isset($handlers[$this->options['tabledrag_hierarchy']['field']])) {
        $errors[] = t('Display "@display": Draggableviews: hierarchy field <i>@field</i> could not be found (..afterwards removed from Fields section?). Check your settings.', array('@display' => $this->display->display_title, '@field' => $this->options['tabledrag_hierarchy']['field']));
      }
    }

    return $errors;
  }

  /**
   * Render the given style.
   */
  function options_form(&$form, &$form_state) {
    // inherit options from style_table
    parent::options_form($form, $form_state);

    // set theme handler
    // theme function is registered in *.module file
    $form['#theme'] = 'draggableviews_ui_style_plugin_draggabletable';

    // get field handler
    $handlers = $this->display->handler->get_handlers('field');


    // DRAGGABLE VIEW OPTIONS
    //
    // + Set field(s) to save the order in
    // + Set field to save the parent in
    // + Apply tabledrag-type to content-types (Root (can't have parent), Leaf (can't have children)).
    // + Set if expand/collapse links should be shown

    $input = array();
    $input = $form_state['input'];

    // get all system-wide node types as a keyed array
    foreach (node_get_types('types') AS $node_type) {
      $node_types[$node_type->type] = $node_type->type;
    }

    $form['tabledrag_hierarchy'] = array();
    $form['tabledrag_order']     = array();
    $form['tabledrag_types']     = array();

    // check for input
    if (count($input) > 0) {
      // define the input data as the current data
      $current = $form_state['input']['style_options'];
    }
    else {
      // define the already stored data as the current data
      $current = $this->options;
    }

    $form['tabledrag_header'] = array(
      '#prefix' => '<div class="">',
      '#suffix' => '</div>',
      '#value' => t('Draggable Table Settings:'),
    );
    $form['tabledrag_desription'] = array(
      '#prefix' => '<div class="description form-item">',
      '#suffix' => '</div>',
      '#value' => t('Specify..'),
    );
    $form['tabledrag_description_types'] = array(
      '#prefix' => '<div class="description form-item">',
      '#suffix' => '</div>',
      '#value' => t('Type "root" cannot be subordinated.<br />Type "leaf" cannot have child nodes.<br />Content types with type "root" assigned will own a link to collapse/expand the subordinated nodes (if checked below).'),
    );
    $form['tabledrag_description_hierarchy'] = array(
      '#prefix' => '<div class="description form-item">',
      '#suffix' => '</div>',
      '#value' => t('Choose a nodereference CCK-field if you want to use hierarchies.'),
    );

    $options        = _draggableviews_filter_fields(array('number'), $handlers);
    $draggableviews_handlers = draggableviews_get_handlers_list();

    $first_option   = each($options);
    $first_handler  = each($draggableviews_handlers);


    // Build order fields
    // These fields will be used to store the weight of each hierarchy level
    $extra_row = isset($input['tabledrag_order_add']) ? TRUE : FALSE;
    for ($i = 0, $index = 0; $i < count($current['tabledrag_order']) + ($extra_row == TRUE ? 1 : 0); $i++) {

      // if option should be deleted, continue loop
      if (isset($input['tabledrag_order_del_'. $i])) continue;

      $form['tabledrag_order'][$index] = array(
        'field' => array(
          '#type' => 'select',
          '#options' => $options,
          '#value' => isset($current['tabledrag_order'][$i]['field']) ? $current['tabledrag_order'][$i]['field'] : $first_option['key'],
        ),
        'handler' => array(
          '#type' => 'select',
          '#options' => $draggableviews_handlers,
          '#value' => isset($current['tabledrag_order'][$i]['handler']) ? $current['tabledrag_order'][$i]['handler'] : $first_handler['key'],
        ),
        'del' => array(
          '#type' => 'button',
          '#name' => 'tabledrag_order_del_'. $index,
          '#value' => t('Delete'),
        ),
      );
      $index++;
    }
    $form['tabledrag_order_add'] = array(
        '#type' => 'button',
        '#name' => 'tabledrag_order_add',
        '#value' => t('Add order field'),
    );

    // TABLEDRAG SHOW/HIDE INPUT FIELDS
    // Build checkbox
    // decide whether order input fields should be visible or not
    $form['tabledrag_order_visible'] = array(
        '#type' => 'checkboxes',
        '#name' => 'tabledrag_order_visible',
        '#options' => array('visible' => 'Show input fields?'),
        '#title' => t('Decide whether order input fields (e.g. weight, parent) should be visible or not'),
        '#default_value' => isset($current['tabledrag_order_visible']) ? $current['tabledrag_order_visible'] : array(),
    );

    $form['tabledrag_hierarchy'] = array(
      '#weight' => 10,
      'field' => array(
        '#type' => 'select',
        '#options' => array('none' => 'none') + _draggableviews_filter_fields(array('nodereference'), $handlers),
        '#value' => isset($current['tabledrag_hierarchy']['field']) ? $current['tabledrag_hierarchy']['field'] : 'none',
      ),
      'handler' => array(
        '#type' => 'select',
        '#options' => $draggableviews_handlers,
        '#value' => isset($current['tabledrag_hierarchy']['handler']) ? $current['tabledrag_hierarchy']['handler'] : $first_handler['key'],
      ),
    );

    // TABLEDRAG SHOW/HIDE INPUT FIELDS
    // Build checkbox
    // decide whether parent input fields should be visible or not
    $form['tabledrag_hierarchy_visible'] = array(
        '#type' => 'checkboxes',
        '#name' => 'tabledrag_hierarchy_visible',
        '#options' => array('visible' => 'Show input fields?'),
        '#title' => t('Decide whether parent input fields (e.g. weight, parent) should be visible or not'),
        '#default_value' => isset($current['tabledrag_hierarchy_visible']) ? $current['tabledrag_hierarchy_visible'] : array(),
    );

    // TABLEDRAG TYPES (ROOT,LEAF)
    // These fields will save the behaviour of a node-type (root, leaf)
    $extra_row = isset($input['tabledrag_types_add']) ? TRUE : FALSE;
    for ($i = 0, $index = 0; $i < count($current['tabledrag_types']) + ($extra_row == TRUE ? 1 : 0); $i++) {

      // if option should be deleted, continue loop
      if (isset($input['tabledrag_types_del_'. $i])) continue;

      $form['tabledrag_types'][$index] = array(
        'node_type' => array(
          '#type' => 'select',
          '#options' => $node_types,
          '#value' => isset($current['tabledrag_types'][$i]['node_type']) ? $current['tabledrag_types'][$i]['node_type'] : $node_types[0],
        ),
        'type' => array(
          '#type' => 'select',
          '#options' => array('root' => 'root', 'leaf' => 'leaf'),
          '#value' => isset($current['tabledrag_types'][$i]['type']) ? $current['tabledrag_types'][$i]['type'] : 'root',
        ),
        'tabledrag_type_del_button' => array(
          '#type' => 'button',
          '#name' => 'tabledrag_types_del_'. $index,
          '#value' => t('Delete'),
        ),
      );
      $index++;
    }
    $form['tabledrag_types_add'] = array(
        '#type' => 'button',
        '#name' => 'tabledrag_types_add',
        '#value' => t('Add type'),
    );

    /* TABLEDRAG EXPAND/COLLAPSE OPTION
     * Build checkbox
     * decide if epand links should be shown
     */
    $form['tabledrag_expand'] = array(
        '#type' => 'checkboxes',
        '#name' => 'tabledrag_expand',
        '#options' => array('expand_links' => 'Show expand Links?', 'collapsed' => 'default is collapsed'),
        '#title' => t('Decide whether expand/collapse Links should be shown or not'),
        '#default_value' => isset($current['tabledrag_expand']) ? $current['tabledrag_expand'] : array('expand_links'),
    );

    // Let extension modules alter the output
    foreach (module_implements('draggableviews_style_plugin_form_alter') as $module) {
      $function = $module .'_draggableviews_style_plugin_form_alter';
      $function($form, $form_state, $this);
    }
  }

  /**
   * Render the draggable table style.
   */
  function render() {
    // We need to wrap around a form to make it possible to submit changes.
    // Due to the fact that multiple views can be shown on the same page we
    // need to use an unique form_id for each view. Same views can share the same id
    // because the concerned views object is the same.
    return drupal_get_form('draggableviews_view_draggabletable_form_'. $this->view->vid .'_'. $this->view->current_display, $this);
  }
}
