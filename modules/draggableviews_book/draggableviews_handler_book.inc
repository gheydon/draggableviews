<?php
// $Id$

/**
 * @file
 * The default implementation for draggableviews.
 */

/*
 * Default Implementation with Books.
 */
class draggableviews_handler_book {
  function init($field_name, &$view) {
    $this->view = $view;

    $this->field_name = $field_name;

    $this->view = &$view;

    if ($field_name == $view->style_plugin->options['tabledrag_hierarchy']['field']) {
      $this->type = 'hierarchy';
      $this->root = 0;
      // Check if filters or arguments define another root parent.
      $options = $view->style_plugin->options;
      if (isset($options['draggableviews_book_radios'])) {
        switch ($options['draggableviews_book_radios']) {
          case 1:
            $argument = $options['draggableviews_book_argument'];
            if (isset($this->view->argument[$argument]->position)) {
              $position = $this->view->argument[$argument]->position;
              $this->root = $this->view->args[$position];
            }
            break;
          case 2:
            $filter = $options['draggableviews_book_filter'];
            if (isset($this->view->filter[$filter]->options['value']['value'])) {
              $this->root = $this->view->filter[$filter]->options['value']['value'];
            }
            break;
        }
      }
    }
    else {
      $this->type = 'order';
    }
  }

  function get($value) {
    if ($this->type == 'hierarchy') {
      if ($value == $this->root) {
        return 0;
      }
    }
    return $value;
  }

  function save($nid, $value) {
    // Load the noad and reset it if it's already cached.
    $node = node_load($nid, NULL, TRUE);
    switch ($this->type) {
      case 'hierarchy':
        // Recover the actual root parent.
        if ($value == 0) $value = $this->root;
        $node->book['plid'] = db_result(db_query("SELECT mlid FROM {book} WHERE nid = %d", $value));
        break;
      case 'order':
        $node->book['weight'] = $value;
        break;
    }
    $node = node_submit($node);
    node_save($node);
  }

  function get_form_element($value, $attributes = array()) {
    switch ($this->type) {
      default:
        $options = array();
        for ($i = $this->range_start; $i <= $this->range_end; $i++) $options[$i] = $i;

        return array(
          '#type'  => 'select',
          '#name'  => $attributes['field_name'],
          '#value' => $value,
          '#options' => $options,
          '#attributes' => array('class' => $attributes['class']),
        );
        break;

      case 'hierarchy':
        return array(
          '#type' => 'hidden',
          '#name' => $attributes['field_name'],
          '#value' => $value,
          '#attributes' => array('class' => $attributes['class']),
        );
    }
  }

  function set_range($start, $end) {
    $this->range_start = $start;
    $this->range_end   = $end;
  }
}
