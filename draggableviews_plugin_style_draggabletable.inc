<?php

/**
 * Style plugin to render each item as a row in a draggable table;
 * Inherits all from views_plugin_table.
 */
class draggableviews_plugin_style_draggabletable extends views_plugin_style_table {
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

    
    /**************************
     * DRAGGABLE VIEW OPTIONS *
     **************************
       + Set tabledrag options (multiple).
       + Set depth and weight-fields of each depth.
       + Apply tabledrag-type to content-types (Root (can't have parent), Leaf (can't have children)).
    */
    
    // get display object
    $displayObj = $this->view->display['default'];
    // define the saved data in the view object as the current data
    $current = $displayObj->display_options['style_options'];
    
    
    // Create an array of allowed columns from the data we know:
    
    // get all system-wide node types as a keyed array
    foreach(node_get_types('types') AS $node_type){     /* get value for each */
      $node_types[$node_type->type] = $node_type->type; /* build new array */
    }
    
    $form['tabledrag_options'] = array();      // set tabledrag options
    $form['tabledrag_depth_fields'] = array(); // set depth fields (hierarchy weights) where weights will be saved
    $form['tabledrag_types'] = array();        // set tabledrag types (root, leaf) for specific node-types
    
    // define some variables
    $leaveIndex = -1;      // index of tabledrag data that should not be left out
                           // (..to delete a specific tabledrag data row)
    $leaveIndex_type = -1; // index of tabledrag types that should not be left out
                           // (..to delete a specific tabledrag type)
    $extra_rows = 0;       // new tabledrag-data-rows to add at end of array
    $extra_rows_type = 0;  // new tabledrag-type-rows to add at end of array
    //check if realtimeedit module exists
    if(module_exists('realtimeedit')){
      // set tabledrag types (root, leaf) for specific node-types
      $form['realtimeedit_enabled_fields'] = array();
      $leaveIndex_realtimeedit = -1; // index of tabledrag types that should not be left out
      $extra_rows_realtimeedit = 0;  // new tabledrag-type-rows to add at end of array
    }
    
    // check for input
    if( count($form_state['input']) > 0 ) {
      // Input is available (form has been submitted) ->
      
      // React on clicked buttons:
      // Button: add action
      if( isset( $form_state['input']['tabledrag_action_add_button'] ) ){
      	$extra_rows = 1; // 1 extra row should be built
      }
      // Buttons: check all tabledrag delete buttons
      for($i = 0; $i < count( $form_state['input']['style_options']['tabledrag_options'] ); $i++){
        // Button: delete action
        if( isset( $form_state['input']['tabledrag_action_del_button_' . $i] ) ){
          $leaveIndex = $i; // set tabledrag index to delete
        }
      }
      
      // Button: add tabledrag type
      if(isset($form_state['input']['tabledrag_type_add_button'])){
      	  $extra_rows_type = 1;
      }
      // Buttons: check all delete buttons
      for($i = 0; $i < count( $form_state['input']['style_options']['tabledrag_types'] ); $i++){
        // Button: delete action
        if( isset( $form_state['input']['tabledrag_type_del_button_' . $i] ) ){
          $leaveIndex_type = $i; // set tabledrag type index to delete
        }
      }
      
      //check if realtimeedit module exists
      if(module_exists('realtimeedit')){
        // Button: add realtimeedit enabled field
        if(isset($form_state['input']['realtimeedit_add_button'])){
          $extra_rows_realtimeedit = 1;
        }
        // Buttons: check all delete buttons
        for($i = 0; $i < count( $form_state['input']['style_options']['realtimeedit_enabled_fields'] ); $i++){
          // Button: delete action
          if( isset( $form_state['input']['realtimeedit_del_button_' . $i] ) ){
            $leaveIndex_realtimeedit = $i; // set tabledrag type index to delete
          }
        }
      }
      
      // define the input data as the current data
      $current = $form_state['input']['style_options'];
    }
    
    
    
    /* Some markup to introduce DRAGGABLE VIEW form elements
     *******************************************************
    */
    $form['head_draggabletable_markup'] = array(
      '#prefix' => '<div class="">',
      '#suffix' => '</div>',
      '#value' => t('Draggable Table Settings:'),
    );
    $form['description_draggabletable_markup'] = array(
      '#prefix' => '<div class="description form-item">',
      '#suffix' => '</div>',
      '#value' => t('Draggable Table description.'),
    );
    $form['head_draggabletable_markup'] = array(
      '#prefix' => '<div class="">',
      '#suffix' => '</div>',
      '#value' => t('Add new tabledrag:'),
    );
    $form['description_draggabletable_content_type'] = array(
      '#prefix' => '<div class="description form-item">',
      '#suffix' => '</div>',
      '#value' => t('Type "root" cannot be subordinated.<br>Type "leaf" cannot have child nodes.<br>Content types with type "root" assigned will own an link to collapse/expand the subordinated nodes (if checked below).'),
    );
    
    
    /* Build tabledrag actions form elements
     *******************************************************
    */
    
    // loop: go through all defined tabldrag options
    for($i = 0, $button_index = 0;                                 // initalize count variable and button index
        $i < count($current['tabledrag_options']) + $extra_rows;   // set break condition (+1 new extra row if desired)
        $i++)                                                      // increase index
    {
      // if tabldrag option should be left out, continue loop
      // (delete tabledrag option from array)
      if($i == $leaveIndex) continue;
      
      // if current table drag option is an extra row,
      // don't use existing data to fill form-elements (because they would not exist yet)
      $overwrite = $i < count($current['tabledrag_options']) ? true : false;
      
      if( $overwrite == true){
        //get options of current table drag
        $tmpOptions = $current['tabledrag_options'][$i];
      }
      
      //build form elements
      $form['tabledrag_options'][] = array(
        '#weight' => 10,
        'tabledrag_action' => array(
          '#type' => 'select',
          '#options' => array('match' => 'match', 'order' => 'order'),
          '#value' => $overwrite ? $tmpOptions['tabledrag_action'] : 'match',
        ),
        'tabledrag_relationship' => array(
          '#type' => 'select',
          '#options' => array('parent' => 'parent', 'sibling' => 'sibling'),
          '#value' => $overwrite ? $tmpOptions['tabledrag_relationship'] : 'parent',
        ),
        /* we don't use the options group and subgroup in this version.
         Maybe we need them in the future.*/
        /*
        'tabledrag_group' => array(
          '#type' => 'select',
          '#options' => $available_fields,
          '#value' => $overwrite ? $tmpOptions['tabledrag_group'] : $available_fields[0],
        ),
        'tabledrag_subgroup' => array(
          '#type' => 'select',
          '#options' => $available_fields,
          '#value' => $overwrite ? $tmpOptions['tabledrag_subgroup'] : $available_fields[0],
        ),
        */
        'tabledrag_source' => array(
          '#type' => 'select',
          '#options' => _draggableviews_filter_fields(array('number', 'nodereference'), $handlers),
          '#value' => $overwrite ? $tmpOptions['tabledrag_source'] : $available_fields[0],
        ),
        'tabledrag_action_del_button' => array(
          '#type' => 'button',
          '#name' => 'tabledrag_action_del_button_' . $button_index,
          '#value' => t('Delete'),
        ),
      );
      
      // increase button index
      $button_index++;
    }
    
    //build button to add a new tabldrag
    $form['tabledrag_action_add_button'] = array(
        '#type' => 'button',
        '#name' => 'tabledrag_action_add_button',
        '#value' => t('Add Tabledrag'),
    );
    
    
    /* Build tabledrag_depth fields
     ***************************************
       These fields will be used to store the weight of each hierarchy.
       This section will not be shown if tabledrag{action:oder,relationship:sibling} is not used
    */
    
    // get tabledrag depth
    $tabledrag_depth=isset($current['tabledrag_depth_fields']) ? $current['tabledrag_depth'] : 1;
    
    // loop: add a select for each depth
    for($i = 0; $i < $tabledrag_depth; $i++){
      // set current depth field
      $tmpDepthField = $current['tabledrag_depth_fields']['field_' . $i];
      
      //SELECT: choose field where weight of depth should be saved
      $form['tabledrag_depth_fields']['field_'.$i] = array(
        '#type' => 'select',
        '#options' => _draggableviews_filter_fields(array('number'), $handlers),
        '#value' => $tmpDepthField ? $tmpDepthField : $available_fields[0],
      );
    }
    
    // Button: set depth
    $form['tabledrag_set_depth_button'] = array(
        '#type' => 'button',
        '#name' => 'button_set_depth',
        '#value' => t('Set depth'),
    );
    
    // SELECT: choose depth
    $form['tabledrag_depth'] = array(
      '#type' => 'select',
      '#options' => array(1 => 1, 2 => 2, 3 => 3, 4 => 4), //DUMMY OPTIONS
      '#default_value' => $tabledrag_depth,
    );
    
    //TABLEDRAG TYPES (ROOT,LEAF)
    /* Build tabledrag type fields
     ***************************************
       These fields will save the behaviour of a node-type (root, leaf)
    */
    
    for($i=0, $button_index=0;                                    // initalize count variable and button index
        $i<count($current['tabledrag_types']) + $extra_rows_type; // set break condition (+1 new extra row if desired)
        $i++)                                                     // increase index
    {
      
      // if tabldrag option should be left out, continue loop
      // (delete tabledrag option from array)
      if($i == $leaveIndex_type) continue;
      
      // if current table drag type is an extra row,
      // don't use existing data to fill form-elements (because they are not existing yet)
      $overwrite = $i < count($current['tabledrag_types']) ? true : false;
      
      //get options of current table drag
      $tmpOtions = $current['tabledrag_types'][$i];
      
      $form['tabledrag_types'][] = array(
        '#weight' => 10,
        'field_type' => array(
          '#type' => 'select',
          '#options' => $node_types,
          '#value' => $overwrite ? $tmpOtions['field_type'] : $node_types[0],
        ),
        'type' => array(
          '#type' => 'select',
          '#options' => array('root' => 'root', 'leaf' => 'leaf'),
          '#value' => $overwrite ? $tmpOtions['type'] : 'root',
        ),
        'tabledrag_type_del_button' => array(
          '#type' => 'button',
          '#name' => 'tabledrag_type_del_button_' . $button_index,
          '#value' => t('Delete'),
        ),
      );
      
      // increase button index
      $button_index++;
    }
    
    $form['tabledrag_type_add_button'] = array(
        '#type' => 'button',
        '#name' => 'tabledrag_type_add_button',
        '#value' => t('Add type'),
    );
    
    //TABLEDRAG EXPAND/COLLAPSE OPTION
    /* Build checkbox
     ***************************************
       decide if epand links should be shown
    */
    $form['tabledrag_expand'] = array(
        '#type' => 'checkboxes',
        '#name' => 'tabledrag_expand',
        '#options' => array('expand_links' => 'Show expand Links?', 'collapsed' => 'default is collapsed'),
        '#title' => t('Decide whether expand/collapse Links should be shown or not'),
        '#default_value' => isset($current['tabledrag_expand']) ? $current['tabledrag_expand'] : array('expand_links'),
    );
    
    //TABLEDRAG SHOW/HIDE INPUT FIELDS
    /* Build checkbox
     ***************************************
       decide whether input fields should be visible or not
    */
    $form['tabledrag_visible'] = array(
        '#type' => 'checkboxes',
        '#name' => 'tabledrag_visible',
        '#options' => array('visible' => 'Show input fields?'),
        '#title' => t('Decide whether input fields (e.g. weight, parent) should be visible or not'),
        '#default_value' => isset($current['tabledrag_visible']) ? $current['tabledrag_visible'] : array(),
    );
    
    
    /*****************************************
     * IMPLEMENTATION OF REALTIMEEDIT MODULE *
    ******************************************/
    
    //check if realtimeedit module exists
    if(module_exists('realtimeedit')){
      // set tabledrag types (root, leaf) for specific node-types
      $form['realtimeedit_enabled_fields'] = array();
      $leaveIndex_type = -1; // index of tabledrag types that should not be left out
                             // (..to delete a specific tabledrag type)
      $extra_rows_type = 0;  // new tabledrag-type-rows to add at end of array
      
      // REALTIMEEDIT ENABLED FIELDS
      /* Build realtimeedit enabled fields
       ***************************************
         These fields will be realtime-editable
      */
      $max_count = count($current['realtimeedit_enabled_fields']) + $extra_rows_realtimeedit;
      $button_index=0; // initalize count variable and button index
      for($i = 0; $i < $max_count; $i++) {
      
        // if realtimeedit enabled field should be left out, continue loop
        // (delete enabled field from array)
        if($i == $leaveIndex_realtimeedit) continue;
        
        // if current enabled field is an extra row,
        // don't use existing data to fill form-elements (because they are not existing yet)
        $overwrite = $i < count($current['realtimeedit_enabled_fields']) ? true : false;
      
        //get options of current enabled field
        $tmpOtions = $current['realtimeedit_enabled_fields'][$i];
      
        $form['realtimeedit_enabled_fields'][] = array(
          '#weight' => 10,
          'field' => array(
            '#type' => 'select',
            '#options' => $available_fields,
            '#value' => $overwrite ? $tmpOtions['field'] : $available_fields[0],
          ),
          'realtimeedit_del_button' => array(
            '#type' => 'button',
            '#name' => 'realtimeedit_del_button_' . $button_index,
            '#value' => t('Delete'),
          ),
        );
      
        // increase button index
        $button_index++;
      }
    
      $form['realtimeedit_add_button'] = array(
        '#type' => 'button',
        '#name' => 'realtimeedit_add_button',
        '#value' => t('Enable new field'),
      );
    }
  }

  /**
   * Render the table style.
   */
  function render() {
    // call form handler to wrap around a form.
    // -> this makes it possible to submit order changes.
    return drupal_get_form('draggableviews_view_draggabletable_form', &$this);
    /*
    $sets = $this->render_grouping($this->view->result, $this->options['grouping']);

    $output = '';
    foreach ($sets as $title => $records) {
      $output .= theme($this->theme_functions(), $this->view, $this->options, $records, $title);
    }
    return $output;*/
  }
}

/**
 * Theme the form for the table style plugin
 */
function theme_draggableviews_ui_style_plugin_draggabletable($form) {
  $output = drupal_render($form['description_markup']);

  $header = array(
    t('Field'),
    t('Column'),
    t('Separator'),
    array(
      'data' => t('Sortable'),
      'align' => 'center',
    ),
    array(
      'data' => t('Default sort'),
      'align' => 'center',
    ),
  );
  $rows = array();
  foreach (element_children($form['columns']) as $id) {
    $row = array();
    $row[] = drupal_render($form['info'][$id]['name']);
    $row[] = drupal_render($form['columns'][$id]);
    $row[] = drupal_render($form['info'][$id]['separator']);
    if (!empty($form['info'][$id]['sortable'])) {
      $row[] = array(
        'data' => drupal_render($form['info'][$id]['sortable']),
        'align' => 'center',
      );
      $row[] = array(
        'data' => drupal_render($form['default'][$id]),
        'align' => 'center',
      );
    }
    else {
      $row[] = '';
      $row[] = '';
    }
    $rows[] = $row;
  }

  // Add the special 'None' row.
  $rows[] = array(t('None'), '', '', '', array('align' => 'center', 'data' => drupal_render($form['default'][-1])));

  $output .= theme('table', $header, $rows);
  
  
  
  /* Render Draggable view settings
   ********************************
  */
  
  // set labels
  $labels=array(t('Action'), t('Relationship'), /*t('Group'), t('Subgroup'),*/ t('Source'), '&nbsp;');
  $labels_types=array(t('Content type'), t('Tabledrag type'), '&nbsp;');
  $labels_orderfields = array(t('Save order in..'));
  // initialize table rows
  $rows=array();
  $rows_types=array();
  $rows_orderfields = array();
  
  $submit_button = drupal_render($form['tabledrag_action_add_button']);   // render submit button
  $add_type_button = drupal_render($form['tabledrag_type_add_button']);   // render add type button
  $set_depth_button = drupal_render($form['tabledrag_set_depth_button']); // render set depth button
  $set_depth_select = drupal_render($form['tabledrag_depth']);            // render depth select element
  $tabledrag_expand = drupal_render($form['tabledrag_expand']);           // render expand link checkbox
  $tabledrag_visible = drupal_render($form['tabledrag_visible']);
  
  $description_content_type = drupal_render($form['description_draggabletable_content_type']);

  // prepare table data:
  
  // assign tabledrag options to an array
  foreach(element_children($form['tabledrag_options']) AS $id){
    $columns = &$form['tabledrag_options'][$id]; // get columns
    
    $tmpRow=array();
    foreach(element_children($columns) AS $el_id){
      // build row
      $tmpRow[] = drupal_render($columns[$el_id]);
    }
    $rows[]=$tmpRow; // append row to array
  }
  
  // assing tabledrag types to an array
  foreach(element_children($form['tabledrag_types']) AS $id){
    $columns=&$form['tabledrag_types'][$id]; // get columns
    
    $tmpRow=array();
    foreach(element_children($columns) AS $el_id){
      $tmpRow[] = drupal_render($columns[$el_id]);
    }
    $rows_types[] = $tmpRow; // append row to array
  }
  // assing tabledrag depth fields to an array
  foreach(element_children($form['tabledrag_depth_fields']) AS $id){
    $column=&$form['tabledrag_depth_fields'][$id]; // get column
    
    $rows_orderfields[] = array(drupal_render($column)); // append row to array
  }
  
  /* IMPLEMENTATION OF REALTIMEEDIT MODULE */
  //check if realtimeedit module exists
  if(module_exists('realtimeedit')){
    $add_realtimeedit_button = drupal_render($form['realtimeedit_add_button']); // render set depth button
    
    //set label
    $labels_realtimeeditfields = array(t('Enabled Fields'), '&nbsp;');
    
    // assing tabledrag depth fields to an array
    foreach(element_children($form['realtimeedit_enabled_fields']) AS $id){
      $columns=&$form['realtimeedit_enabled_fields'][$id]; // get columns
      
      $tmpRow=array();
      foreach(element_children($columns) AS $el_id){
        $tmpRow[] = drupal_render($columns[$el_id]);
      }
      $rows_realtimeedit_fields[] = $tmpRow; // append row to array
    }
  }
  
  // add form data left to output
  $output .= drupal_render($form);
  
  // add tabledrags to output
  $output .= theme('table', $labels, $rows);
  $output .= $submit_button;
  
  // add tabledrag types to output (choose if type = leaf|root)
  $output .= theme('table', $labels_types, $rows_types);
  $output .= $add_type_button;
  $output .= $description_content_type;
  
  // add tabledrag order depth table to output
  $output .= theme('table', $labels_orderfields, $rows_orderfields);
  $output .= theme('table', array("",""), array( array($set_depth_select, $set_depth_button) ) );
  
  // add expand yes/no checkbox to output
  $output .= $tabledrag_expand;
  $output .= $tabledrag_expand_default;
  
  // add visible yes/no checkbox to output
  $output .= $tabledrag_visible;
  
  /* IMPLEMENTATION OF REALTIMEEDIT MODULE */
  //check if realtimeedit module exists
  if(module_exists('realtimeedit')){
    // add editable fields table to output
    $output .= theme('table', $labels_realtimeeditfields, $rows_realtimeedit_fields);
    // add button
    $output .= $add_realtimeedit_button;
  }
  
  // return output
  return $output;
}