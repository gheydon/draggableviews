<?php

namespace Drupal\Tests\draggableviews\Functional;

use Drupal\Tests\BrowserTestBase;
use Drupal\node\Entity\Node;

/**
 * Tests sortability of Draggableviewws.
 *
 * @group draggableviews
 */

class DraggableviewsTest extends BrowserTestBase {
  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = ['node', 'views', 'draggableviews', 'draggableviews_demo'];

  /**
   * The installation profile to use with this test.
   *
   * @var string
   */
  protected $profile = 'minimal';

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    // Create users.
    $this->adminUser = $this->drupalCreateUser([
      'access administration pages',
      'view the administration theme',
      'administer permissions',
      'administer nodes',
      'administer content types',
	  'access draggableviews',
    ]);
    $this->authUser = $this->drupalCreateUser([], 'authuser');

    // Gather the test data.
    $dataContent = $this->providerTestDataContent();
    
    // Create nodes. 
    foreach ($dataContent as $datumContent) {
      $node = $this->drupalCreateNode([
        'type' => 'draggableviews_demo',
        'title' => $datumContent[0],
        'body' => [
          'format' => filter_default_format($this->authUser),
          'value' => $datumContent[1]
        ]
      ]);
      $node->save();
    }
  }

  /**
   * Data provider for setUp
   *
   * @return array
   *  Nested array of testing data, Arranged like this:
   *  - Title
   *  - Body
   */
  protected function providerTestDataContent() {
    return [
      [
        'Draggable Content 1',
        'Draggable Content Body 1',
      ],
      [
        'Draggable Content 2',
        'Draggable Content Body 2',
      ],
      [
        'Draggable Content 3',
        'Draggable Content Body 3',
      ],
      [
        'Draggable Content 4',
        'Draggable Content Body 4',
      ],
      [
        'Draggable Content 5',
        'Draggable Content Body 5',
      ]
    ];
  }
  /**
   * A simple test.
   */
  public function testDraggableviewsContent() {
	$assert_session = $this->assertSession();
	
	$this->drupalGet('draggableviews-demo');
    $this->assertSession()->statusCodeEquals(200);
	// Verify that anonymous useres cannot access the order page.
	$this->drupalGet('draggableviews-demo/order');
    $this->assertSession()->statusCodeEquals(403);
	
	// Verify that authorized user has access to display and order page.
    $this->drupalLogin($this->adminUser);
    $this->drupalGet('draggableviews-demo');
    $this->assertSession()->statusCodeEquals(200);
    $this->drupalGet('draggableviews-demo/order');
    $this->assertSession()->statusCodeEquals(200);
	
	// Verify that the page contains generated content.
	// $assert_session->pageTextContains(t('Draggable Content 5'));
  }
}
