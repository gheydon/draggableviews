<?php
/**
 * Display a view as a draggable table style.
 */

function template_preprocess_draggableviews_view_draggabletable(&$vars) {
  $view     = $vars['view'];

  // We need the raw data for this grouping, which is passed in as $vars['rows'].
  // However, the template also needs to use for the rendered fields.  We
  // therefore swap the raw data out to a new variable and reset $vars['rows']
  // so that it can get rebuilt.
  $result   = $vars['rows'];
  $vars['rows'] = array();

  $options  = $view->style_plugin->options;
  $handler  = $view->style_plugin;

  $fields   = &$view->field;
  $columns  = $handler->sanitize_columns($options['columns'], $fields);

  $active   = !empty($handler->active) ? $handler->active : '';
  $order    = !empty($handler->order) ? $handler->order : 'asc';

  $query    = tablesort_get_querystring();
  if ($query) {
    $query = '&' . $query;
  }

  foreach ($columns as $field => $column) {
    // render the header labels
    if ($field == $column && empty($fields[$field]->options['exclude'])) {
      $label = check_plain(!empty($fields[$field]) ? $fields[$field]->label() : '');
      if (empty($options['info'][$field]['sortable'])) {
        $vars['header'][$field] = $label;
      }
      else {
        // @todo -- make this a setting
        $initial = 'asc';

        if ($active == $field && $order == 'asc') {
          $initial = 'desc';
        }

        $image = theme('tablesort_indicator', $initial);
        $title = t('sort by @s', array('@s' => $label));
        $link_options = array(
          'html' => true,
          'attributes' => array('title' => $title),
          'query' => 'order=' . urlencode($field) . '&sort=' . $initial . $query,
        );
        $vars['header'][$field] = l($label . $image, $_GET['q'], $link_options);
      }
    }

    // Create a second variable so we can easily find what fields we have and what the
    // CSS classes should be.
    $vars['fields'][$field] = views_css_safe($field);
    if ($active == $field) {
      $vars['fields'][$field] .= ' active';
    }
    
    // Render each field into its appropriate column.
    foreach ($result as $num => $row) {
      if (!empty($fields[$field]) && empty($fields[$field]->options['exclude'])) {
        $field_output = $fields[$field]->theme($row);

        // Don't bother with separators and stuff if the field does not show up.
        if (!isset($field_output) && isset($vars['rows'][$num][$column])) {
          continue;
        }

        // Place the field into the column, along with an optional separator.
        if (isset($vars['rows'][$num][$column])) {
          if (!empty($options['info'][$column]['separator'])) {
            $vars['rows'][$num][$column] .= filter_xss_admin($options['info'][$column]['separator']);
          }
        }
        else {
          $vars['rows'][$num][$column] = '';
        }

        $vars['rows'][$num][$column] .= $field_output;
      }
    }
  }

  $vars['class'] = 'views-table';
  if (!empty($options['sticky'])) {
    drupal_add_js('misc/tableheader.js');
    $vars['class'] .= " sticky-enabled";
  }
  
  
  

  /*********************
   * TABLE_DRAG OUTPUT *
  **********************/
  
  // get style options
  $style_options = $vars['view']->display['default']->display_options['style_options'];
  // check for available tabledrag options
  if( ! isset($style_options['tabledrag_options']) ){
    // if there are no tabledrag options defined
    // we have finished -> return
    return;
  }
  // get tabledrag options
  $tabledrag_options = $style_options['tabledrag_options'];
  
  // check for not supported settings
  $match_nmbr = 0;
  foreach($tabledrag_options as $tabledrag) {
    if ($tabledrag['tabledrag_action'] == 'match') $match_nmbr++;
  }
  if ($match_nmbr > 1) {
    // more than 1 match-action is not supported in this version
    drupal_set_message(t("<strong>Multiple matching is not supported</strong> in this version. There were $match_nmbr tabledrag options found that use action=match.<br/><i>Draggableview</i> will not take effect. Please change style-plugin settings."), 'error');
    return;
  }
  
  //get node type specifications
  $tabledrag_types = $style_options['tabledrag_types'];
  //get depth weight fields
  $tabledrag_depth_fields = $style_options['tabledrag_depth_fields'];
    
  // get node type sepcification
  $node_type_spec = array();
  if( count($tabledrag_types) > 0 ){
    foreach($tabledrag_types as $tabledrag_type){
      $node_type_spec[$tabledrag_type['field_type']] = $tabledrag_type['type'];
    }
  }
  
  // some variables
  $match_used = false; // need to check if tabledrag{action:match;relationship:parent) is beeing used

  // output data
  $vars['tabledrag']         = array();                        // tabledrag settings
  $vars['tabledrag_tableId'] = 'draggableview_' . $view->name; // set table id
  
  // prepare tabledrag settings for output
  foreach($tabledrag_options as $tabledrag) {
    $vars['tabledrag'][] = array(
      'source' => $tabledrag['tabledrag_action']=='match' ? 'hidden_nid' : $tabledrag['tabledrag_source'],
      /* Tabledrag action:match uses the node id to refernce the parent node.
         The nid - if needed - will be provided as a hidden field. */
      'action' => $tabledrag['tabledrag_action'],
      'relationship' => $tabledrag['tabledrag_relationship'],
      'group' => $tabledrag['tabledrag_source'],    // @todo $tabledrag['tabledrag_group'],
      'subgroup' => $tabledrag['tabledrag_source'], // @todo $tabledrag['tabledrag_subgroup'],
      /* we don't use the options group and subgroup in this version.
         Maybe we need them in the future.*/
      'hidden' => $style_options['tabledrag_visible']['visible'] ? FALSE : TRUE,
      'limit' => count($tabledrag_depth_fields)-1, // set limit to number of specified depth fields -1.
                                                   // It wouldn't make sense to allow a dragging to a level
                                                   // that cannot be saved. (0 would mean unlimited)
    );
    
    //find out if {action:match,relationship:parent} is used
    if( $tabledrag['tabledrag_action'] == 'match' &&    // check if tabledrag{action:match}
        $tabledrag['tabledrag_relationship']=='parent') // check if tabledrag{relationship:sibling}
    {
      $match_used = true; // notice that action:match is beeing used
      
      // get name of field where the parent id is saved in
      // (In case of tabledrag{action:match,relationship:parent} the tabledrag_source field
      //  describes the field that contains the parent node)
      $tabledrag_parent_field_name = $tabledrag['tabledrag_source'];
      
      // get field alias
      $tabledrag_parent_field_alias=$fields[$tabledrag_parent_field_name]->field_alias;
      
      // let realtimeedit module know about fields
      drupal_add_js(array('draggableviews' => array($tabledrag['tabledrag_relationship'] => $tabledrag['tabledrag_source'])), 'setting');
      
      // break now as we don't support more than one parent type
      break;
    }
  }
  
  /* Make selected fields editable
   *******************************
   */
  foreach($vars['rows'] as $num => $row){
    $node_id = $result[$num]->nid;                  // get node id
    $tmpNode = node_load(array('nid' => $node_id)); // load node
    
    // set node type specification, if available (such as 'tabledrag-root')
    if( isset($node_type_spec[$tmpNode->type]) ){
      $vars['tabledrag_type'][$num] = 'tabledrag-' . $node_type_spec[$tmpNode->type];
    }
    
    // get hierarchy depth of current row
    $depth = $match_used ? _draggableviews_get_hierarchy_depth($node_id, $result, $tabledrag_parent_field_alias) : 0;
    
    // build indentation (as drupal-tabledrag requires)
    $indentation = theme('indentation', $depth);
    
    // rebuild fields as form elements
    reset($tabledrag_options);
    
    foreach($tabledrag_options as $tabledrag) {
      
      // get field information
      $field_name = $tabledrag['tabledrag_source'];
      $real_field_name = $fields[$field_name]->content_field['field_name'];
      
      /* get field alias
       * we need to determine the views field alias to get access to the fields value
       */
      
      /* unset field alias, so we can check later with
       * isset(..) whether a field alias was found or not */
      unset($field_alias);
      
      if ($depth == 0 ||
         ($tabledrag['tabledrag_action'] == 'match' || $tabledrag['tabledrag_relationship'] == 'parent'))
      {
        // if tabledrag{action:match;relationship:sibling} is present
        // or hierarchy depth = 0
        // always use tabledrag_source (specified in tabledrag-options)
          	
        // set field alias
        $field_alias = $fields[$field_name]->field_alias;
      }else{
        // else (tabledrag{action:sibling;relationship:order} is present)
        // use weight field of highest hierarchy level (as drupal-tabledrag requires)
        
      	// check if field exists
        if( isset($fields[$tabledrag_depth_fields['field_' . $depth]]->field_alias)){
          // set field alias
          $field_alias = $fields[$tabledrag_depth_fields['field_' . $depth]]->field_alias;
        }
        else {
          drupal_set_message(t('There is no field for depth='.$depth.' specified. Check the style-plugin settings.'), 'error');
        }
      }
      
      // now we are able to get the value of the result..
      // ..by using the accurate field alias we determined before
      if( isset($field_alias) ){
        $value = $result[$num]->$field_alias;
        
        if ($tabledrag['tabledrag_relationship'] == 'parent') {
          // in case of relationship:parent we can be sure the should be a nodereference
          // check if node still exists. It could had been deleted!
          if (!node_load(array('nid' => $value))) {
            // if parent node doesn't exist any more unset value
            unset($value);
          }
        }
      }else{
        // if field alias is empty
        drupal_set_message('$field_alias not available ($result['.$num.']->'.$field_alias.').', 'error');
      }
      
      // get accurate form element depending on field type
      $form_element = _draggableviews_get_form_element($node_id, $field_name, $real_field_name, $value/*, &$view*/);
      
      // render new form element
      $vars['rows'][$num][$field_name] = drupal_render($form_element);
    }
    
    // if required add node id field
    if($match_used == true){
      // build form element
      $hidden_nid = array(
        '#type' => 'hidden',
        '#name' => 'hidden_nid',
        '#value' => $node_id,
        '#attributes' => array('class' => 'hidden_nid'),
      );
      // append rendered hidden node id to last rendered field
      $vars['rows'][$num][$field_name] .= drupal_render($hidden_nid);
      
      // put indentation in front of first rendered field
      $first_field = each($vars['rows'][$num]);
      $vars['rows'][$num][$first_field['key']] = $indentation . $vars['rows'][$num][$first_field['key']];
    }
    
    
    /*****************************************
     * IMPLEMENTATION OF REALTIMEEDIT MODULE *
    ******************************************/
    
    if(module_exists('realtimeedit') &&  // check if module exists
       user_access('administer nodes'))  // check permissions
    {
      //get enabled fields
      $realtimeedit_enabled_fields = $style_options['realtimeedit_enabled_fields'];
      
      // get realtimeedit enabled fields sepcification
      $enabled_fields_spec = array();
      if( count($realtimeedit_enabled_fields) > 0 ){
        foreach($realtimeedit_enabled_fields as $enabled_field){
          $enabled_fields[$enabled_field['field']] = true;
        }
      }
        
      // prepare all fields
      foreach($fields AS $key => $field_obj){
        if( $enabled_fields[$key] &&               // check if field is marked to be editable
            $field_obj->options['exclude'] == 0 )  // check if field is not exluded from display
        {
          //watchdog('special', '<pre>'.print_r($field_obj, true).'</pre>');
          // set unique selector
          $css_class = 'realtimeedit-field';
          $selector = 'realtimeedit-field-' . $node_id . '-' . $field_obj->definition['content_field_name'] . '-alias-' . $key;
          $wrapper_id = $selector . '-wrapper';
          $ahah_binding = array(
            'url'   => url('realtimeedit/ajax'),
            'event' => 'click',
            'wrapper' => $wrapper_id,
            'selector' => '#' . $selector,
            'effect'   => 'fade',
            'method'   => 'replace',
            'progress' => array('type' => 'throbber'),
          );
          // add js
          drupal_add_js('misc/jquery.form.js');
          drupal_add_js('misc/ahah.js');
          drupal_add_js(array('ahah' => array($selector => $ahah_binding)), 'setting');

          
          // append rendered hidden node id to last rendered field
          // check if field uses link
          if( $field_obj->options['link_to_node'] == 0 ){
            $vars['rows'][$num][$key] = '<span id="' . $wrapper_id . '" class="' . $css_class . '">' . $vars['rows'][$num][$key] . '</span>';
            //$vars['rows'][$num][$key] .= '<a href="#" id="' . $selector . '">edit</span>';
          }
        }
      }
    }
  }
}

function template_preprocess_draggableviews_view_draggabletable_form($vars){
  global $user;
  
  // get view object
  $viewObj = $vars['form']['#parameters'][2];
  
  // add javascript
  // BE AWARE: realtimeedit module js has to be loaded first,
  // generated links should be appended in the right order
  
  //include realtimeedit module
  if(module_exists('realtimeedit')){ // check if module exists
    drupal_add_js(drupal_get_path('module', 'realtimeedit').'/realtimeedit.js');
    drupal_add_css(drupal_get_path('module', 'realtimeedit').'/styles.css');
  }
  
  // fetch expand information from database
  $result = db_query(db_rewrite_sql("SELECT parent_nid, collapsed FROM {draggableviews_collapsed} WHERE uid = %d"), $user->uid);
  $states = array();
  while ($state = db_fetch_object($result)) {
    $states[$state->parent_nid] = $state->collapsed;
  }
  // check if "expand" links should be shown
  if ($viewObj->options['tabledrag_expand']['expand_links']) {
  
    drupal_add_js(drupal_get_path('module', 'draggableviews').'/draggableviews.js');
    
    if (count($states) == 0) {
      // let js know wether child nodes should be expanded or not
      drupal_add_js(array(
                          'draggableviews' => array(
                            'expand_default' =>  $viewObj->options['tabledrag_expand']['collapsed']
                                                 ? 1 : 0,
                            ),
                          ),
                    'setting');
    }
    else {
      drupal_add_js(array(
                          'draggableviews' => array(
                            'states' => $states,
                            ),
                          ),
                    'setting');
    }
    
    drupal_add_css(drupal_get_path('module', 'draggableviews').'/styles.css');
  }
  
  
  //theme view
  $sets = $viewObj->render_grouping($viewObj->view->result, $viewObj->options['grouping']);
  
  $output = '';
  foreach ($sets as $title => $records) {
    $output .= theme($viewObj->theme_functions(), $viewObj->view, $viewObj->options, $records, $title);
  }
  
  $vars['view'] = $output;

  
  //render submit form
  $vars['submit_form'] = drupal_render($vars['form']);
}