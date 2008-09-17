<?php
// $Id$

include_once('draggableviews_theme.inc');

/**
 * @file
 * Implements preprocess function hook_submit for draggable views
 */
function draggableviews_view_draggabletable_form($form_state, $view){
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}


/**
 * Implementing hook_submit 
 */
function draggableviews_view_draggabletable_form_submit($vars){
  //check permissions
  if(!user_access('administer nodes')){
    drupal_set_message(t('You are not allowed to edit/reorder these nodes.'), 'error');
    return;
  }

  $viewObj = $vars['#parameters'][2]; // get view object
  $results = $viewObj->view->result;  // get results
  $fields  = $viewObj->view->field;   // get fields
  $input = $vars['submit']['#post'];  // get input
  
  /*
  $inputs = array(
    'parents' => array(
      'sajdkflsjakdf' => array(
        43 => 15,
        44 => 15,
      ),
    ),
    'levels' => array(
      1 => array(
        43 => -3,
        44 => -2,
      ),
      2 => array(
        43 => -2,
        44 => -1,
      ),
    ),
  );
  */
  
  // get style options
  $style_options=$viewObj->view->display['default']->display_options['style_options'];
  // check for available tabledrag options
  if( ! isset($style_options['tabledrag_options']) ){
    // if there are no tabledrag options defined
    // we have finished -> return
    return;
  }
  // get tabledrag options
  $tabledrag_options = $style_options['tabledrag_options'];

  // some variables
  $match_used = false; // need to check if tabledrag{action:match;relationship:parent) is beeing used
  
  // find out if {action:match,relationship:parent} is used
  foreach($tabledrag_options as $tabledrag){
    if( $tabledrag['tabledrag_action'] == 'match' &&      // check if tabledrag{action:match}
        $tabledrag['tabledrag_relationship'] == 'parent') // check if tabledrag{relationship:sibling}
    {
      $match_used = true; // notice that action:match is beeing used
      
      // get name of field where the parent id is saved in
      // (In case of $match_used=TRUE the tabledrag_source field
      //  describes the field that contains the parent node)
      $tabledrag_parent_field_name = $tabledrag['tabledrag_source'];
      
      // get actual field name
      $tabledrag_real_parent_field_name = $fields[$tabledrag_parent_field_name]->content_field['field_name'];
      
      // break now as we don't support more than one parent type
      break;
    }
  }
  
  // iterate through all tabledrag options
  foreach($tabledrag_options as $tabledrag) {
  
    // get content field
  	$content_field = $fields[$tabledrag['tabledrag_source']]->content_field;
  	
    $real_field_name   = $content_field['field_name']; // get actual field name
    $field_type        = $content_field['type'];       // get actual field type
    
    
    // go through all rows resulting from the view
    foreach($results as $res){
      
      // load node
      $node = node_load(array('nid' => $res->nid));
      
      if ($tabledrag['tabledrag_action'] == 'match' ||
          $tabledrag['tabledrag_relationship'] == 'parent')
      {
        // {action:match,relationship:parent} is present, so set parents
        
        // build field key
        $tmp_field_key = $tabledrag['tabledrag_source'] . '_' . $node->nid;
        // get submitted parent value
        $value         = $input[$tmp_field_key];
        
        // set new weight to node
        // ($real_field_name and $field_type defined above)
        _draggableviews_node_set_value($node, $real_field_name, $field_type, $value);
        
      }
      else { 
        // no matching is present, so set weight
      
        /* calculate depth of current node *
         ***********************************/
        $depth = 0;
        // if {action:match,relationship:parent} is present
        // and current table drag is neither action:match nor relationship:parent
        if($match_used) {
        	
          $tmpNid = $res->nid; // use a temporary node id
          $depth  = -1;        // set depth
          
          /* It might seem confusing that depth is set to -1.
           * But we'll enter the loop at least once for sure ->
           * hence the depth finally will be positive.
           */
          
          // check if nid of current node exists
          while($tmpNid > 0){
            // increase depth
            $depth++;
            
            // prepare new node id (use parent node id) for the next loop cycle
            $tmpNid = $input[$tabledrag_parent_field_name . '_' . $tmpNid];
          }
        }
        
        //if current node's depth == 0 skip the following section
        if($depth > 0){
          /* assign weights to the specified depth-weight-fields,
           * depending on their depth
           ********************************************************/
      	  
          // use a temporary node id
          $tmpNid = $res->nid;
        
          // iterate down to the root level
          for($i = $depth; $i >= 0; $i--){
            /* Start in the deepest level.
             * The weight of the deepest level (level=depth) will be set to the submitted weight of the current node.
             * The weight of the level above (level=depth-1) will be set to the weight of its parent
             * The weight of the level       (level=depth-2) will be set to the parent's parent node...
             * ...
             * Due to the fact that we iterate TOP-DOWN we are able to collect the parent's weights by the way.
             * 
             * Unused depth-weight-fields will be set to the minimum value. Because of this convention
             * we can assure that the rows will be shown in the expected order.
             * (..provided the accurate sort-criterias are set)
             */
            
            // check if desired weight field exists
            if( isset($style_options['tabledrag_depth_fields']['field_'.$i]) ){
              // set field of level $i
              
              // build field key
              $tmp_field_key = $tabledrag['tabledrag_source'] . '_' . $tmpNid;
              // get submitted weight value
              $weight        = $input[$tmp_field_key];
              
              // get field name
              $tmp_field_name      = $style_options['tabledrag_depth_fields']['field_' . $i];
              // get actual field name
              $tmp_real_field_name = $fields[$tmp_field_name]->content_field['field_name'];
              // get field type
              $tmp_field_type      = $fields[$tmp_field_name]->content_field['type'];
              
              // write new weight to node
              _draggableviews_node_set_value($node, $tmp_real_field_name, $tmp_field_type, $weight);
            }
            
            // prepare new node id (use parent node id) for the next loop cycle
            $tmpNid = $input[$tabledrag_parent_field_name . '_' . $tmpNid];
            
            //end of loop
          }
        
        }else{
          /* if no {action:match,relationship:parent} defined
             OR depth == 0 (caused by $match_used=FALSE)
             we don't need to set multiple weights of certain levels
             So we just have to save the submitted weight
          */
      	  
          // build field key
          $tmp_field_key = $tabledrag['tabledrag_source'] . '_' . $res->nid;
          // get submitted weight value
          $value         = $input[$tmp_field_key];
        
          // set new weight to node
          // ($real_field_name and $field_type defined above)
          _draggableviews_node_set_value($node, $real_field_name, $field_type, $value);

          }
        /* Set all unused depth-weight-fields to the minimum value.
           This should prevent that a parent appears under its child
           (because parent and child have the same depth-weight on the parents level)
        */
          
        for($i = count($style_options['tabledrag_depth_fields']) - 1; $i > $depth; $i--) {
          // get field key
          $tmp_field_key       = $style_options['tabledrag_depth_fields']['field_' . $i];
          // get actual field name
          $tmp_real_field_name = $fields[$tmp_field_key]->content_field['field_name'];
          // get field type
          $tmp_field_type      = $fields[$tmp_field_key]->content_field['type'];
          
          // get minimum
          $minimum_value = _draggableviews_field_get_minimum_value($fields[$tmp_field_key]);
          
          // write minimum to node
          _draggableviews_node_set_value($node, $tmp_real_field_name, $tmp_field_type, $minimum_value - 1);
        }
      }
      
      // finally save node
      if($node = node_submit($node)){
        $og_groups = $node->og_groups;
        $nid = $node->nid;
        
        node_save($node); // CAUTION, BUG: node_save destroys og_groups array
       
        // assing og groups manually
        if(isset($og_groups) && is_array($og_groups)){
          $sql = "INSERT INTO {og_ancestry} (nid, group_nid, is_public) VALUES (%d, %d, 0)";
          foreach( $og_groups AS $gid ){
            db_query($sql, $node->nid, $gid);
          }
        }
      }
    }
  }
  
  // save expanded/collapsed state
  global $user;
  
  foreach ($vars['submit']['#post'] AS $key => $val) {
    if (ereg('draggableviews_collapsed_', $key)){
      $parent_nid = substr($key, 25);
      db_query("DELETE FROM {draggableviews_collapsed}
                WHERE uid=" . $user->uid . " AND 
                parent_nid=" . $parent_nid);

      db_query("INSERT INTO {draggableviews_collapsed}
                (uid, parent_nid, collapsed) VALUES (%d, %d, %d)",
                $user->uid, $parent_nid, $val);
    }
  }
}

/*
 * Get index of result array where nid matches
 * - Compare the nid field with the node id we're looking for.
 *   In case of equality return the index of the result array
 * 
 * @param $nid
 *   The node id we're looking for
 * @param $result
 *   The results the view returned
 */
function _draggableviews_get_result_index_by_nid($nid, $result){
  for($i = 0; $i < count($result); $i++){
  	// if current nid matches nid we're looking for, return index
    if($result[$i]->nid == $nid) return $i;
  }
  
  return FALSE;
}

/*
 * Get Hierarchy depth by checking the parent id
 * - (1) Look for a parent node id.
 *   (2) Use new parent node as new node id.
 *   (3) When new node id differs from 0 go back to (1)
 * 
 * @param $node_id
 *   The node id we start from
 * @param $result
 *   The results the view returned
 * @param $parent_field_alias
 *   The field_alias of the field that contains the parent node id
 * return
 *   The hierarchy depth
 */
function _draggableviews_get_hierarchy_depth($node_id, $result, $parent_field_alias){
  $tmpId = $node_id; // set temporary id
  $depth = -1;       // set depth

  //check if nid of current node exists
  while( $tmpId > 0 /* prevent infinity loop in case of failure */&& $depth < 100 /**/){
    $depth++;
    // get index where nid matches tmpId
    if ($index = _draggableviews_get_result_index_by_nid($tmpId, $result)) {
      // get parent id
      $tmpId = $result[$index]->$parent_field_alias;
    }
    else {
      break;
    }
  }
    
  return $depth;
}

/*
 * Get accurate form element
 * 
 * @param $nid
 * @param $field_name
 * @param $value
 * @param $view
 *   The view object
 */
function _draggableviews_get_form_element($nid, $field_name, $real_field_name, $value,/* $view,*/ $specific_field_name = NULL){
  //watchdog('special', '<pre>'.print_r($view, true).'</pre>');
  // get content field
  //print_r($view->field[$field_name]);
  // get node type
  $node = node_load($nid);
  $type = content_types($node->type);
  $field = $type['fields'][$real_field_name];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$field['widget']['type']];
  
  //$content_field = $view->field[$field_name]->content_field;
  // get cck widget type
  //$widget_type = !empty($content_field) ? $content_field['widget']['type'] : 'hidden';
  //watchdog('special', '<pre>'.print_r($view, true).'</pre>');
  switch($field['widget']['type']){
  	case 'optionwidgets_select':
  	  return array(
  	    '#type' => 'select',
  	    '#name' => $specific_field_name != NULL ? $specific_field_name: $field_name . '_' . $nid,
  	    '#value' => $value,
  	    '#options' => content_allowed_values($field),
  	    '#attributes' => array('class' => $field_name),
  	  );
  	  break;
  	case '__nodereference_select':
  	  return array(
  	    '#type' => 'select',
  	    '#name' => $specific_field_name != NULL ? $specific_field_name: $field_name . '_' . $nid,
  	    '#value' => $value,
  	    '#options' => content_allowed_values($field),
  	    '#attributes' => array('class' => $field_name),
  	  );
    return array
        (
            '#theme' => 'content_multiple_values',
            '#title' => 'parent',
            '#required' => 0,
            '#description' => NULL, 
            0 => array
                (
                    '#type' => 'nodereference_autocomplete',
                    '#default_value' => array
                        (
                            'nid' => NULL,
                            '_error_element' => 'default_value_widget][field_parent][0][nid][nid',
                        ),

                    '#value_callback' => 'nodereference_autocomplete_value',
                    '#title' => 'parent',
                    '#description' => NULL,
                    '#required' => NULL,
                    '#weight' => 0,
                    '#delta' => 0,
                    '#columns' => array
                        (
                            0 => 'nid',
                        ),

                    '#field_name' => 'field_parent',
                    '#type_name' => 'task',
                ),

            '#field_name' => 'field_parent',
            '#tree' => 1,
            '#weight' => 3,
            '#access' => 1,
            '#count' => 5,
        );
  	  
  	case 'hidden':
  	default:
      return array(
        '#type' => 'hidden',
        '#name' => $field_name . '_' . $nid,
        '#value' => $value,
        '#attributes' => array('class' => $field_name),
      );
  }
}

/*
 * Set cck fields in a node with a specific field-type
 * 
 * @param $node
 *   The node which contains the cck fields
 * @param $field_name
 *   The cck_field_name that should be set
 * @param $field_type
 *   The field-type of the cck field
 * @param $value
 */
function _draggableviews_node_set_value(&$node, $field_name, $field_type, $value){
  // get field
  if (isset($node->$field_name)) {
    $field = &$node->$field_name;
  }
  else {
    // return if field does not exist
    return;
  }
  
  if (!isset($value)) {
    // sometimes there is no value available (e.g. a root without a parent)
    
    // set empty array
    $field = array();
  }
  
  //differ between certain field types
  switch($field_type){
    case 'nodereference':
      $field[0]['nid'] = $value;
      break;
    
    default:
    case 'number_integer':
      $field[0]['value'] = $value;
      break;
  }
}

/*
 * get the minimum allowed value of a cck-field  
 * 
 * @param $field
 *   the cck fields
 */
function _draggableviews_field_get_minimum_value($field){
  // get allowed values and return first element found
  $allowed_values = each(content_allowed_values($field->content_field));
  
  return $allowed_values['key'];
}

/*
 * filter handlers by type  
 * 
 * @param $type
 *   the field type
 * @param $fields
 *   the fields array
 * return
 *   the available fields
 */
function _draggableviews_filter_fields($types = array(), $handlers) {

  $available_fields = array();
  
  foreach ($handlers as $field => $handler) {
    $available = FALSE;
    // search given type in content-field-type
    if (isset($handler->content_field)) {
      foreach ($types AS $type) {
        if (ereg($type, $handler->content_field['type'])) {
          $available = TRUE;
          break;
        }
      }
    
      if ($available) {
        if ($label = $handler->label()) {
          $available_fields[$field] = $label;
        }
        else {
          $available_fields[$field] = $handler->ui_name();
        }
      }
    }
  }
  return $available_fields;
}

/*
 * Implementing hook_views_pre_view
 */
/*
function draggableviews_views_pre_view($view, $display_id, $view_args){
  // check  if view uses our style plugin
  if( $view->display[$display_id]->handler->get_option('style_plugin') == 'draggabletable' ){
  	
    // Add the node id field if it's not yet present.
    if( !array_key_exists('nid', $view->display[$display_id]->display_options['fields']) ){
      $view->add_item($display_id, 'field', 'node', 'nid');
    }
  }
}*/
?>