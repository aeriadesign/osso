<?php
// $Id: FeedsSimplePieParser.inc,v 1.8 2010/01/12 16:28:02 alexb Exp $

/**
 * Adapter to present SimplePie_Enclosure as FeedsEnclosure object.
 */
class FeedsSimplePieEnclosure extends FeedsEnclosure {
  protected $simplepie_enclosure;

  /**
   * Constructor requires SimplePie enclosure object.
   */
  function __construct(SimplePie_Enclosure $enclosure) {
    $this->simplepie_enclosure = $enclosure;
  }

  /**
   * Override parent::getValue().
   */
  public function getValue() {
    return $this->simplepie_enclosure->get_link();
  }

  /**
   * Override parent::getMIMEType().
   */
  public function getMIMEType() {
    return $this->simplepie_enclosure->get_real_type();
  }
}

/**
 * Class definition for Common Syndication Parser.
 *
 * Parses RSS and Atom feeds.
 */
class FeedsSimplePieParser extends FeedsParser {

  /**
   * Implementation of FeedsParser::parse().
   */
  public function parse(FeedsImportBatch $batch, FeedsSource $source) {
    feeds_include_library('simplepie.inc', 'simplepie');
    
    // Initialize SimplePie.
    $parser = new SimplePie();
    $parser->set_raw_data($batch->getRaw());
    $parser->set_stupidly_fast(TRUE);
    $parser->encode_instead_of_strip(FALSE);
    // @todo Is caching effective when we pass in raw data?
    $parser->enable_cache(TRUE);
    $parser->set_cache_location($this->cacheDirectory());
    $parser->init();

    // Construct the standard form of the parsed feed
    $batch->setTitle(($title = $parser->get_title()) ? $title : $this->createTitle($parser->get_description()));
    $batch->setDescription($parser->get_description());
    $batch->setLink(html_entity_decode($parser->get_link()));

    $items_num = $parser->get_item_quantity();
    for ($i = 0; $i < $items_num; $i++) {
      $item = array();
      $simplepie_item = $parser->get_item($i);
      $item['title'] = ($title = $simplepie_item->get_title()) ? $title : $this->createTitle($simplepie_item->get_content());
      $item['description'] = $simplepie_item->get_content();
      $item['url'] = html_entity_decode($simplepie_item->get_link());
      // Use UNIX time. If no date is defined, fall back to FEEDS_REQUEST_TIME.
      $item['timestamp'] = $simplepie_item->get_date("U");
      if (empty($item['timestamp'])) {
        $item['timestamp'] = FEEDS_REQUEST_TIME;
      }
      $item['guid'] = $simplepie_item->get_id();
      // Use URL as GUID if there is no GUID.
      if (empty($item['guid'])) {
        $item['guid'] = $item['url'];
      }
      $author = $simplepie_item->get_author();
      $item['author_name'] = $author->name;
      $item['author_link'] = $author->link;
      $item['author_email'] = $author->email;
      // Enclosures
      $enclosures = $simplepie_item->get_enclosures();
      if (is_array($enclosures)) {
        foreach ($enclosures as $enclosure) {
          $item['enclosures'][] = new FeedsSimplePieEnclosure($enclosure);
        }
      }
      // Location
      $latitude = $simplepie_item->get_latitude();
      $longitude = $simplepie_item->get_longitude();
      if (!is_null($latitude) && !is_null($longitude)) {
        $item['location_latitude'][] = $latitude;
        $item['location_longitude'][] = $longitude;
      }
      // Extract tags related to the item
      $simplepie_tags = $simplepie_item->get_categories();
      $tags = array();
      $domains = array();
      if (count($simplepie_tags) > 0) {
        foreach ($simplepie_tags as $tag) {
          $tags[] = (string) $tag->term;
          $domain = (string) $tag->get_scheme();
          if (!empty($domain)) {
            if (!isset($domains[$domain])) {
              $domains[$domain] = array();
            }
            $domains[$domain][] = count($tags) - 1;
          }
        }
      }
      $item['domains'] = $domains;
      $item['tags'] = $tags;
      // Stick the raw data onto the feed item.
      $item['raw'] = $simplepie_item->data;
      $batch->addItem($item);
    }
    // Release parser.
    unset($parser);
  }

  /**
   * Return mapping sources.
   */
  public function getMappingSources() {
    return array(
      'title' => array(
        'name' => t('Title'),
        'description' => t('Title of the feed item.'),
      ),
      'description' => array(
        'name' => t('Description'),
        'description' => t('Description of the feed item.'),
      ),
      'author_name' => array(
        'name' => t('Author name'),
        'description' => t('Name of the feed item\'s author.'),
      ),
      'author_link' => array(
        'name' => t('Author link'),
        'description' => t('Link to the feed item\'s author.'),
      ),
      'author_email' => array(
        'name' => t('Author email'),
        'description' => t('Email address of the feed item\'s author.'),
      ),
      'timestamp' => array(
        'name' => t('Published date'),
        'description' => t('Published date as UNIX time GMT of the feed item.'),
      ),
      'url' => array(
        'name' => t('Item URL (link)'),
        'description' => t('URL of the feed item.'),
      ),
      'guid' => array(
        'name' => t('Item GUID'),
        'description' => t('Global Unique Identifier of the feed item.'),
      ),
      'tags' => array(
        'name' => t('Categories'),
        'description' => t('An array of categories that have been assigned to the feed item.'),
      ),
      'domains' => array(
        'name' => t('Category domains'),
        'description' => t('Domains of the categories.'),
      ),
      'location_latitude' => array(
        'name' => t('Latitudes'),
        'description' => t('An array of latitudes assigned to the feed item.'),
      ),
      'location_longitude' => array(
        'name' => t('Longitudes'),
        'description' => t('An array of longitudes assigned to the feed item.'),
      ),
      'enclosures' => array(
        'name' => t('Enclosures'),
        'description' => t('An array of enclosures attached to the feed item.'),
      ),
     );
  }

  /**
   * Returns cache directory. Creates it if it doesn't exist.
   */
  protected function cacheDirectory() {
    $directory = file_directory_path() .'/simplepie';
    file_check_directory($directory, TRUE);
    return $directory;
  }

  /**
   * Generate a title from a random text.
   */
  protected function createTitle($text = FALSE) {
    // Explode to words and use the first 3 words.
    $words = preg_split("/[\s,]+/", $text);
    $words = array_slice($words, 0, 3);
    return implode(' ', $words);
  }
}