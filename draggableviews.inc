<?php
// $Id$

/**
 * @file
 * Draggableviews processing functions.
 * Rough summary of what functions in this file do:
 *  - Fetch views-style-plugin information
 *  - Analyze structure
 *  - Build/rebuild hierarchy
 */

/**
 * Fetch views-style-plugin information.
 * Collect all known information in a handy array.
 *
 * @param $view
 *   The views object
 *
 * @return
 *   A structured array containing the extracted draggableviews settings, additional
 *   field information, all nodes the view returned and the view object itself.
 *   array(
 *     'view' => view,
 * 
 *     'order' => array(
 *       'fields' => array(
 *         0 => array(
 *           'field_name'  => field_name
 *           'field_alias' => field_alias
 *           'handler'     => handler,
 *         ),
 *         ..
 *       ),
 *       'visible' => True/False
 *       'minimum_value' => value
 *     ),
 * 
 *     'hierarchy' => array(
 *       'field' => array(
 *         'field_name'  => field_name,
 *         'field_alias' => field_alias,
 *         'handler'     => handler,
 *       ),
 *       'visible' => TRUE/FALSE,
 *     ),
 *
 *     'types' = array(
 *       node_type => "root"/"leaf",
 *     ),
 *
 *     'expand_links' = array(
 *       'show'              => TRUE/FALSE,
 *       'default_collapsed' => TRUE/FALSE,
 *     ),
 *
 *     'nodes' => array(
 *       nid1 => array(
 *         'order' => array(
 *           0 => value,
 *           1 => value,
 *           ..
 *         ),
 *         'parent' => value,
 *       ),
 *       ..
 *     ),
 *   );
 */
function _draggableviews_info($view, $info = NULL) {
  $options = $view->style_plugin->options;
  $fields  = $view->field;
  $results = $view->result;

  // if there is already an info array just rebuild the nodes array and skip this section
  if (!isset($info)) {
    $info = array();

    // extract draggableviews settings.
    if (isset($options['tabledrag_order'])) {
      foreach ($options['tabledrag_order'] as $field) {
        if ($handler = _draggableviews_init_handler($field, $view)) {
          $info['order']['fields'][] = array(
            'handler' => $handler,
            'field_name' => $field['field'],
            'field_alias' => $fields[$field['field']]->field_alias,
          );
        }
        else {
          drupal_set_message(t('Draggableviews: Handler <i>') . $field['handler'] . t('</i> could not be found.'), 'error');
          unset($info['order']);
          return $info;
        }
      }
      $info['order']['visible'] = strcmp($options['tabledrag_order_visible']['visible'], 'visible') == 0 ? TRUE : FALSE;
      $info['order']['minimum_value'] = $info['order']['fields'][0]['handler']->get_minimum_value();

      if (count($info['order']['fields']) >= 2 && $options['tabledrag_hierarchy']['field'] != 'none') {
        if ($handler = _draggableviews_init_handler($options['tabledrag_hierarchy'], $view)) {
          $info['hierarchy'] = array(
            'field' => array(
              'handler' => $handler,
              'field_name' => $options['tabledrag_hierarchy']['field'],
              'field_alias' => $fields[$options['tabledrag_hierarchy']['field']]->field_alias,
            ),
            'visible' => strcmp($options['tabledrag_hierarchy_visible']['visible'], 'visible') == 0 ? TRUE : FALSE,
          );

          if (isset($options['tabledrag_types'])) {
            foreach ($options['tabledrag_types'] as $type) {
              $info['types'][$type['node_type']] = $type['type'];
            }
          }

          $info['expand_links'] = array(
            'show'              => strcmp($options['tabledrag_expand']['expand_links'], 'expand_links') == 0 ? TRUE : FALSE,
            'default_collapsed' => strcmp($options['tabledrag_expand']['collapsed'], 'collapsed') == 0 ? TRUE : FALSE,
          );
        }
        else {
          drupal_set_message(t('Draggableviews: Handler <i>') . $field['handler'] . t('</i> could not be found.'), 'error');
        }
      }
    }
  }

  // Hold a reference to the view object itself.
  $info['view'] = &$view;

  // Get all nodes and their properties.
  $info['nodes'] = array();
  if ($info['order']) {
    // loop through all resulting nodes
    foreach ($results as $row) {
      $node = array();
      foreach ($info['order']['fields'] as $field) {
        $info['nodes'][$row->nid]['order'][] = $row->{$field['field_alias']};
      }
      if (isset($info['hierarchy'])) {
        $info['nodes'][$row->nid]['parent'] = $row->{$info['hierarchy']['field']['field_alias']};
      }
    }
  }

  return $info;
}

/*
 * Quick Check Structure
 * 
 * We use this function because of performance issues. Enhanced checks will be done when we have to
 * rebuild a broken structure.
 * 
 * Loop through all nodes.
 * The order values must increase by 1, beginning new with each hierarchy level.
 * We check for the following:
 *  - Wrong order values: The order value should be the line_number. At the place of children must be gaps (on the root level).
 *    Line based numbering is necessary since this module supports paging.
 *  - Parent mismatch: The parent_nid must equal with the nid we memorized before we entered the current hierarchy level.
 * 
 * @param inputs
 *   The structured info array. Look at _draggableviews_info(..) to learn more.
 * 
 * return
 *   TRUE if structure is valid
 */
function _draggableviews_quick_check_structure($info) {
  // calculate views page offset
  $pager  = $info['view']->pager;
  $offset = $pager['items_per_page'] * $pager['current_page'] + $pager['offset'];

  return _draggableviews_ascending_numbers($info, $offset, FALSE);
}

/**
 * Build hierarchy
 * 
 * Although there shouldn't be any structure based errors after submit
 * broken parents can be detected and repaired. All other structure based errors
 * can not be detected. If there are any they will be detected by _draggableviews_quick_check_structure(..)
 * and finally repaired by _draggableviews_analyze_structure(..).
 *
 * @param info
 *   The structured information array
 */
function _draggableviews_build_hierarchy(&$info) {
  $nodes = &$info['nodes'];
  $input = &$info['input'];

  foreach ($nodes as $nid => $prop) {
    // get depth
    if (($depth = _draggableviews_get_hierarchy_depth($nid, $input, $info)) === FALSE) {
      // Error! The hierarchy structure is broken and could
      // look like the following: (we're currently processing X)
      // A
      //   --X
      // --D
      //
      // The next steps:
      //  1) bring it down to the root level
      //  2) Set order fields to the minimum

      $input[$nid]['parent'] = 0;

      // We gracefully sidestep the order-loop
      $depth = -1;
    }

    // Let's take a look at the following expample, to understand
    // what is beeing done.
    //
    // A
    // --B
    // --C
    //   --X
    // --D
    // E
    // Imagine we're currently processing X:
    //
    // We know that X is in depth=3, so we save the received
    // weight value in the 3rd order field of node X.
    //
    // The 2nd order field must inherit the received weight of
    // node C (the next parent). And the 1st order field must
    // inherit the received weight of node A (the parent of C).
    //
    // When we finally order the view by weight1, weight2, weight3 then
    // weight1 and weight2 from node X will always equal with
    // those from node A and B, and weight3 defines the order of the 3rd level.

    $temp_nid = $nid;

    for ($i = $depth; $i >= 0; $i--) {
      // we're operating top-down, so we determine the parents nid by the way

      $nodes[$nid]['order'][$i] = $input[$temp_nid]['order'][0];

      if (isset($info['hierarchy']) && $i > 0) {
        if (!($temp_nid = $input[$temp_nid]['parent'])) {
          // this line should never be reached assumed the depth
          // was calculated correctly.
          drupal_set_message(t('Undefined State called in draggableviews_build_hierarchy(..)'), 'error');
          break;
        }
      }
    }

    if (isset($info['hierarchy'])) {
      // Simply set the parent value
      $nodes[$nid]['parent'] = $input[$nid]['parent'];
    }

    // Now set all unused weights to a minimum value. Otherwise
    // it could happen that a child appears above its parent.
    // The ? can be anything, unfortunately also > 5
    //
    // --A (3,5)
    // B   (3,?)
    //
    // To guaranteer that the ? is always the lowest, we choose
    // the minimum.

    $depth = ($depth == -1) ? 0 : $depth;

    for ($i = $depth + 1; $i < count($info['order']['fields']); $i++) {
      $nodes[$nid]['order'][$i] = $info['order']['fields'][$i]['handler']->get_minimum_value();
      $nodes[$nid]['depth']     = $depth;
    }
  }
  
  // Last but not least sort nodes and assign ascending numbers. This is necessary since this module supports paging.
  _draggableviews_sort_nodes($nodes);
  // calculate views page offset
  $pager  = $info['view']->pager;
  $offset = $pager['items_per_page'] * $pager['current_page'] + $pager['offset'];
  _draggableviews_ascending_numbers($info, $offset, TRUE);
}

/**
 * Rebuild hierarchy
 *
 * This function is called when the structure is broken
 *
 * @param info
 *   The structures information array
 */
function _draggableviews_rebuild_hierarchy(&$info) {
  // We have to make sure that there's no hidden node with an order value that refers to the current page.
  // If the items to display per page are limitated we load the entire view.
  $pager = $info['view']->pager;
  if ($pager['items_per_page'] > 0) {
    _draggableviews_reload_info($info, 0, 0, 0);
  }

  // Calculate depth values.
  // Nodes with broken parents will be brought down to the root level.
  // These depth values will be used for both theming and repairing broken structures.
  _draggableviews_calculate_depths($info);

  // Detect and repair ordering errors.
  // The nodes order values have to equal with the parents order values.
  _draggableviews_check_order($info);

  // The last issue we have to deal with is the order itself. Probably there
  // are more than 1 nodes with the same order value which could lead to display errors.
  // We just sort the nodes by their current order values and assign subsequent ascending numbers.
  _draggableviews_sort_nodes($info['nodes']);
  _draggableviews_ascending_numbers($info, 0, TRUE);
  
  // Save hierarchy.
  _draggableviews_save_hierarchy($info);

  // The structure should be valid now.
  // Nonetheless let's make a final check for debugging reasons.
  if (!_draggableviews_quick_check_structure($info)) {
    drupal_set_message("Draggableviews: Rebuilding structure didn't work. The structure is broken.", "error");
  }

  // Re-execute view with original page settings.
  _draggableviews_reload_info($info, $pager['items_per_page'], $pager['current_page'], $pager['offset']);
}

/**
 * Calculate Depths
 * 
 * Nodes with broken parents will be brought down to the root level.
 *
 * @param info
 *   The structured information array. Look at _draggableviews_info(..) to learn more.
 *
 * @return
 *   TRUE if no errors occured.
 */
function _draggableviews_calculate_depths(&$info) {
  $error = FALSE;

  // Loop through all rows the view returned.
  foreach ($info['nodes'] as $nid => $prop) {
    // determine hierarchy depth of current row
    $info['nodes'][$nid]['depth'] = _draggableviews_get_hierarchy_depth($nid, $info['nodes'], $info);

    if ($info['nodes'][$nid]['depth'] === FALSE) {
      $error = TRUE;
      $info['nodes'][$nid]['depth'] = 0;
    }
  }

  return !$error;
}

/**
 * Get Hierarchy Depth
 *
 * This function detects broken parents.
 * Cycles are detected (if a child is the parent of its parent).
 *
 * @param $nid
 *   The nid of the node which should be processed.
 * @param $nodes
 *   The nodes array. This can be both a nodes array or an input array.
 *   They share the same hierarchy properties.
 * @param $info
 *   The structured info array. Look at _draggableviews_info(..) to learn more.
 * 
 * return
 *   The hierarchy depth if no occured, else FALSE.
 */
function _draggableviews_get_hierarchy_depth($nid, $nodes, $info) {
  if (!isset($info['hierarchy'])) return 0;

  $depth     = 0;
  $error     = FALSE;
  $temp_nid  = $nid;
  $used_nids = array($temp_nid);

  while (!($error = !isset($nodes[$temp_nid])) && ($temp_nid = $nodes[$temp_nid]['parent']) > 0) {
    // In order to detect cycles we use an array,
    // where all used nids are saved in. If a node id
    // occurs twice -> return FALSE
    $cycle_found = array_search($temp_nid, $used_nids);
    // The isset(..) is necessary because in PHP-Versions <= 4.2.0 array_search() returns NULL
    // instead of FALSE if $temp_nid was not found.
    if (isset($cycle_found) && $cycle_found !== FALSE) {
      drupal_set_message(t("Draggableviews: A cycle was found."));
      return FALSE;
    }

    $used_nids[] = $temp_nid;
    $depth++;
  }

  if ($error) {
    // If loop breaked caused by an error
    return FALSE;
  }

  return $depth;
}

/*
 * Detect and Repair Order Values
 * 
 * The nodes order value has to equal with the parents order value.
 * 
 * @param info
 *   The structured information array. Look at _draggableviews_info(..) to learn more.
 */
function _draggableviews_check_order(&$info) {
  $error = FALSE;
  foreach ($info['nodes'] as $nid => $prop) {
    if (_draggableviews_check_node_order($nid, $info) == FALSE) {
      $error = TRUE;
    }
  }

  return $error;
}

/**
 * Check Node Order Value
 * 
 * We rely on the correctness of depth values and parent values.
 *
 * @param nid
 *   The node id to check.
 * @param info
 *   The structured information array. Look at _draggableviews_info(..) to learn more.
 *
 * @return
 *   TRUE if no errors were found.
 */
function _draggableviews_check_node_order($nid, &$info) {
  $error = FALSE;
  $nodes = &$info['nodes'];

  $temp_nid = $nodes[$nid]['parent'];

  for ($i = $nodes[$nid]['depth'] - 1; $i >= 0; $i--) {
    // we're operating top-down, so we determine the parents nid by the way

    if ($nodes[$nid]['order'][$i] != $nodes[$temp_nid]['order'][$i]) {
      $nodes[$nid]['order'][$i] = $nodes[$temp_nid]['order'][$i];
      $error = TRUE;
    }
    
    $temp_nid = $nodes[$temp_nid]['parent'];
  }

  return !$error;
}

/**
 * Save nodes
 *
 * @param $info
 *   The structured information array
 */
function _draggableviews_save_hierarchy($info) {

  // loop through all nodes
  foreach ($info['nodes'] as $nid => $prop) {

    if (isset($info['hierarchy'])) {
      $value   = $prop['parent'];
      $handler = $info['hierarchy']['field']['handler'];
      $handler->save($nid, $value);
    }

    for ($i = 0; $i < count($info['order']['fields']); $i++) {
      // loop through all order fields
      $value   = $prop['order'][$i];
      $handler = $info['order']['fields'][$i]['handler'];
      $handler->save($nid, $value);
    }
  }
}

/**
 * Click Sort
 * 
 * The nodes are already in the right order.
 * Simply assign ascending values.
 * Re-execute view and refresh info array.
 *
 * @param $info
 *   The structured info array. Look at _draggableviews_info(..) to learn more.
 */
function _draggableviews_click_sort(&$info) {
  // If the items to display per page are limitated we load the entire view.
  $pager = $info['view']->pager;
  if ($pager['items_per_page'] > 0) {
    _draggableviews_reload_info($info, 0, 0, 0);
  }

  // First bring all child nodes down to the root level. We can't keep the hierarchy information
  // because the nodes have been mixed up thoroughly.
  if (isset($info['hierarchy'])) {
    foreach ($info['nodes'] AS $nid => $values) {
      $info['nodes'][$nid]['parent'] = 0;
      $info['nodes'][$nid]['depth']  = 0;
    }
  }
  // Assign ascending numbers
  _draggableviews_ascending_numbers($info, 0, TRUE);
  _draggableviews_save_hierarchy($info);

  // Re-execute view with original page settings.
  _draggableviews_reload_info($info, $pager['items_per_page'], $pager['current_page'], $pager['offset']);
}

/**
 * (CHECK/WRITE) Ascending Numbers.
 * 
 * This function is used for both renumbering and checking.
 * 
 * @param $write
 *   Renumber or check.
 *
 * @param $nodes
 *   The structured info array. Look at _draggableviews_info(..) to learn more.
 */
function _draggableviews_ascending_numbers(&$info, $offset = 0, $renumber = FALSE) {
  // We need to hold
  //  1)  the last nid,
  //  2)  the last order value,
  //  3)  the parent's nids of each level,
  //  3a) the current depth (hierarchy level),
  //  4)  the line number (it is needed since this module supports paging).
  $last_nid        = -1;
  $last_order      = array($offset);
  $last_parent_nid = array(0);
  $depth           = 0;

  foreach ($info['nodes'] as $nid => $values) {
    $parent_nid = isset($info['hierarchy']) ? $values['parent'] : 0;

    // Check if we leave the hierarchy level. Probably we leave more than one level.
    $leave_hierarchy = 0;
    for ($i = 0; $i < $depth; $i++) {
      if ($parent_nid == $last_parent_nid[$i]) {
        // Set the numbers of levels we are leaving.
        $leave_hierarchy = $depth - $i;
        break;
      }
    }
    if ($leave_hierarchy > 0) {
      // We leave this hierarchy level.
      for ($i = 0; $i < $leave_hierarchy; $i++) {
        array_pop($last_parent_nid);
        $depth--;
      }
      $last_order[$depth] = $last_order[$leave_hierarchy];
      array_pop($last_order);
    }
    else if ($parent_nid == $last_nid) {
      // This is the first child of the last node.
      // We enter a new hierarchy level.
      array_push($last_parent_nid, $last_nid);
      array_push($last_order, $last_order[$depth]);
      // Hold for the last level the previous order value.
      $last_order[$depth]--;
      $depth++;
    }
    else if ($parent_nid != $last_parent_nid[$depth]) {
      // This node is neither a member of the current hierarchy level
      // nor a member of the previous hierarchy level
      // nor a child of the last node (opening a new level).
      // There's something wrong!
      drupal_set_message("Something wrong in _draggableviews_ascending_numbers (". ($renumber ? 'WRITE' : 'CHECK').") .", "error");
      return FALSE;
    }

    // This function is used for both renumbering and checking the hierarchy.
    if ($renumber) {
      // Assign order values of parent nodes.
      for ($i = 0; $i <= $depth; $i++) {
        $info['nodes'][$nid]['order'][$i] = $last_order[$i];
      }
    }
    else {
      // Check structure.
      $order = $values['order'][$depth];
      if ($order != $last_order[$depth]) {
        // This would cause trouble with paging.
        // We better initiate a rebuild!
        return FALSE;
      }
    }

    $last_nid = $nid;
    $last_order[$depth]++;
  }

  return TRUE;
}

/**
 * Sort Nodes
 * 
 * @param $nodes
 *   The nodes array to sort
 */
function _draggableviews_sort_nodes(&$nodes) {
  uasort($nodes, '_draggableviews_compare_nodes');
}

/**
 * Compare Nodes (used by usort)
 * 
 *
 * @param $node1
 * @param $node2
 * 
 * @return
 *   < or > or ==
 */
function _draggableviews_compare_nodes($node1, $node2) {
  // The first difference will be significant. If they equal in each level we
  // "survive" the loop and end up with return 0.
  for ($i = 0; $i < count($node1['order']); $i++) {
    // First compare the order values..
    if ($node1['order'][$i] < $node2['order'][$i]) return -1;
    if ($node1['order'][$i] > $node2['order'][$i]) return 1;
    // ..then let child nodes appear under parent nodes.
    if ($node1['parent'] < $node2['parent']) return -1;
    if ($node1['parent'] > $node2['parent']) return 1;
  }
  return 0;
}

/**
 * Extend view window
 * 
 * In order to support paging we have to extend the visible window of each page.
 * We also have to make sure that parent nodes appear with all their children (..and child nodes with their parents).
 *
 * @param $info
 *   The structured info array. Look at _draggableviews_info(..) to learn more.
 * @param $offset_top
 * @param $offset_bottom
 */
function _draggableviews_extend_view_window(&$info, $offset_top, $offset_bottom) {
  $items_per_page = $info['view']->pager['items_per_page'];
  $current_page   = $info['view']->pager['current_page'];

  $index_first = $items_per_page * $current_page - $offset_top;
  $index_last  = $items_per_page * $current_page + $items_per_page - 1 + $offset_bottom;

  if ($index_first < 0) {
    $index_first = 0;
    $offset_top = 0;
  }

  if ($info['hierarchy']) {
    // Load view and info array without paging options.
    _draggableviews_reload_info($info, 0, 0);
    $nodes = $info['nodes'];

    // Get the first node to show.
    for ($i = 0; $i < $index_first; $i++) next($nodes);
    $tmp_node = current($nodes);
    if ($tmp_node['parent'] > 0) {
      // Look for the parent.
      while (($tmp_node = prev($nodes)) !== FALSE) {
        $offset_top++;
        if ($tmp_node['parent'] == 0) break;
      }
      $index_first = $items_per_page * $current_page - $offset_top;
    }
    if ($index_last < count($nodes) - 1) {
      // Get the last node to show.
      for ($i = 0; $i < $index_last - $index_first; $i++) next($nodes);
      while (($tmp_node = next($nodes)) !== FALSE) {
        if ($tmp_node['parent'] == 0) break;
        $offset_bottom++;
      }
      $index_last  = $items_per_page * $current_page + $items_per_page - 1 + $offset_bottom;
    }
    else {
      $index_last = count($nodes) - 1;
      $offset_bottom = 0;
    }
  }

  // We need to add 1 to the number of items to show (e.g. show from index 5 to index 5: show 5-5+1=1 items).
  _draggableviews_reload_info($info, $index_last - $index_first + 1, 0, $index_first);

  // Set extension tag to extended nodes.
  for ($i = 0; $i < $offset_top; $i++) {
    $nid = key($info['nodes']);
    next($info['nodes']);
    $info['nodes'][$nid]['extension'] = TRUE;
  }
  end($info['nodes']);
  for ($i = 0; $i < $offset_bottom; $i++) {
    $nid = key($info['nodes']);
    prev($info['nodes']);
    $info['nodes'][$nid]['extension'] = TRUE;
  }

  for ($i = 0; $i < count($info['order']['fields']); $i++) {
    $info['order']['fields'][$i]['handler']->set_range($index_first, $index_last);
  }
}

/**
 * Reload Info Array
 * 
 * @param info
 *   The structured info array. Look at _draggableviews_info(..) to learn more.
 * @param items_per_page
 * @param current_page
 */
function _draggableviews_reload_info(&$info, $items_per_page = NULL, $current_page = NULL, $offset = NULL) {
  _draggableviews_re_execute_view($info['view'], $items_per_page, $current_page, $offset);
  $info = _draggableviews_info($info['view'], $info);
}

/**
 * Re-execute View
 * 
 * @param view
 *   The view object.
 * @param items_per_page
 * @param current_page
 */
function _draggableviews_re_execute_view(&$view, $items_per_page = NULL, $current_page = NULL, $offset = NULL) {
  if (isset($items_per_page)) $view->pager['items_per_page'] = $items_per_page;
  if (isset($current_page))   $view->pager['current_page']   = $current_page;
  if (isset($offset))         $view->pager['offset']         = $offset;
  $originial_page = $_GET['page'];
  $_GET['page'] = $current_page;
  $view->executed = FALSE;
  $view->execute();
  $_GET['page'] = $original_page;
}

/**
 * Get Handler Instance
 *
 * @param field
 *   The field options specified in the style plugin
 * @param view
 *   The view object
 *
 * @return
 *   A handler instance.
 */
function _draggableviews_init_handler($field, &$view) {
  if (isset($field['handler'])) {
  	$handler_info = draggableviews_discover_handlers($field['handler']);
  	$file         = $handler_info['path'] .'/'. $handler_info['file'];
  	if ($handler_info['path'] && $handler_info['file'] && file_exists($file)) {
      require_once($file);
      $handler = new $handler_info['handler'];
      $handler->init($field['field'], $view);
      return $handler;
  	}
  }
  else {
    return FALSE;
  }
}
