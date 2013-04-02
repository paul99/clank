// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Javascript for omnibox.html, served from chrome://omnibox/
 * This is used to debug omnibox ranking.  The user enters some text
 * into a box, submits it, and then sees lots of debug information
 * from the autocompleter that shows what omnibox would do with that
 * input.
 *
 * The simple object defined in this javascript file listens for
 * certain events on omnibox.html, sends (when appropriate) the
 * input text to C++ code to start the omnibox autcomplete controller
 * working, and listens from callbacks from the C++ code saying that
 * results are available.  When results (possibly intermediate ones)
 * are available, the Javascript formats them and displays them.
 */
cr.define('omniboxDebug', function() {
  'use strict';

  /**
   * Register our event handlers.
   */
  function initialize() {
    $('omnibox-input-form').addEventListener(
        'submit', startOmniboxQuery, false);
    $('prevent-inline-autocomplete').addEventListener(
        'change', startOmniboxQuery);
    $('prefer-keyword').addEventListener('change', startOmniboxQuery);
    $('show-details').addEventListener('change', refresh);
    $('show-incomplete-results').addEventListener('change', refresh);
    $('show-all-providers').addEventListener('change', refresh);
  }

  /**
   * @type {Array.<Object>} an array of all autocomplete results we've seen
   *     for this query.  We append to this list once for every call to
   *     handleNewAutocompleteResult.  For details on the structure of
   *     the object inside, see the comments by addResultToOutput.
   */
  var progressiveAutocompleteResults = [];

  /**
   * @type {number} the value for cursor position we sent with the most
   *     recent request.  We need to remember this in order to display it
   *     in the output; otherwise it's hard or impossible to determine
   *     from screen captures or print-to-PDFs.
   */
  var cursorPositionUsed = -1;

  /**
   * Extracts the input text from the text field and sends it to the
   * C++ portion of chrome to handle.  The C++ code will iteratively
   * call handleNewAutocompleteResult as results come in.
   */
  function startOmniboxQuery(event) {
    // First, clear the results of past calls (if any).
    progressiveAutocompleteResults = [];
    // Then, call chrome with a four-element list:
    // - first element: the value in the text box
    // - second element: the location of the cursor in the text box
    // - third element: the value of prevent-inline-autocomplete
    // - forth element: the value of prefer-keyword
    cursorPositionUsed = $('input-text').selectionEnd;
    chrome.send('startOmniboxQuery', [
        $('input-text').value,
        cursorPositionUsed,
        $('prevent-inline-autocomplete').checked,
        $('prefer-keyword').checked]);
    // Cancel the submit action.  i.e., don't submit the form.  (We handle
    // display the results solely with Javascript.)
    event.preventDefault();
  }

  /**
   * Returns a simple object with information about how to display an
   * autocomplete result data field.
   * @param {string} header the label for the top of the column/table.
   * @param {string} urlLabelForHeader the URL that the header should point
   *     to (if non-empty).
   * @param {string} propertyName the name of the property in the autocomplete
   *     result record that we lookup.
   * @param {boolean} displayAlways whether the property should be displayed
   *     regardless of whether we're in detailed more.
   * @param {string} tooltip a description of the property that will be
   *     presented as a tooltip when the mouse is hovered over the column title.
   * @constructor
   */
  function PresentationInfoRecord(header, url, propertyName, displayAlways,
                                  tooltip) {
    this.header = header;
    this.urlLabelForHeader = url;
    this.propertyName = propertyName;
    this.displayAlways = displayAlways;
    this.tooltip = tooltip;
  }

  /**
   * A constant that's used to decide what autocomplete result
   * properties to output in what order.  This is an array of
   * PresentationInfoRecord() objects; for details see that
   * function.
   * @type {Array.<Object>}
   * @const
   */
  var PROPERTY_OUTPUT_ORDER = [
    new PresentationInfoRecord('Provider', '', 'provider_name', true,
        'The AutocompleteProvider suggesting this result.'),
    new PresentationInfoRecord('Type', '', 'type', true,
        'The type of the result.'),
    new PresentationInfoRecord('Relevance', '', 'relevance', true,
        'The result score. Higher is more relevant.'),
    new PresentationInfoRecord('Contents', '', 'contents', true,
        'The text that is presented identifying the result.'),
    new PresentationInfoRecord('Starred', '', 'starred', false,
        'A green checkmark indicates that the result has been bookmarked.'),
    new PresentationInfoRecord(
        'HWYT', '', 'is_history_what_you_typed_match', false,
        'A green checkmark indicates that the result is an History What You ' +
        'Typed Match'),
    new PresentationInfoRecord('Description', '', 'description', false,
        'The page title of the result.'),
    new PresentationInfoRecord('URL', '', 'destination_url', true,
        'The URL for the result.'),
    new PresentationInfoRecord('Fill Into Edit', '', 'fill_into_edit', false,
        'The text shown in the omnibox when the result is selected.'),
    new PresentationInfoRecord(
        'IAO', '', 'inline_autocomplete_offset', false,
        'The Inline Autocomplete Offset.'),
    new PresentationInfoRecord('Del', '', 'deletable', false,
        'A green checkmark indicates that the results can be deleted from ' +
        'the visit history.'),
    new PresentationInfoRecord('Prev', '', 'from_previous', false, ''),
    new PresentationInfoRecord(
        'Tran',
        'http://code.google.com/codesearch#OAMlx_jo-ck/src/content/public/' +
        'common/page_transition_types.h&exact_package=chromium&l=24',
        'transition', false,
        'How the user got to the result.'),
    new PresentationInfoRecord(
        'Done', '', 'provider_done', false,
        'A green checkmark indicates that the provider is done looking for ' +
        'more results.'),
    new PresentationInfoRecord(
        'Template URL', '', 'template_url', false, ''),
    new PresentationInfoRecord(
        'Associated Keyword', '', 'associated_keyword', false, ''),
    new PresentationInfoRecord(
        'Additional Info', '', 'additional_info', false,
        'Provider-specific information about the result.')
  ];

  /**
   * Returns an HTML Element of type table row that contains the
   * headers we'll use for labeling the columns.  If we're in
   * detailed_mode, we use all the headers.  If not, we only use ones
   * marked displayAlways.
   */
  function createAutocompleteResultTableHeader() {
    var row = document.createElement('tr');
    var inDetailedMode = $('show-details').checked;
    for (var i = 0; i < PROPERTY_OUTPUT_ORDER.length; i++) {
      if (inDetailedMode || PROPERTY_OUTPUT_ORDER[i].displayAlways) {
        var headerCell = document.createElement('th');
        if (PROPERTY_OUTPUT_ORDER[i].urlLabelForHeader != '') {
          // Wrap header text in URL.
          var linkNode = document.createElement('a');
          linkNode.href = PROPERTY_OUTPUT_ORDER[i].urlLabelForHeader;
          linkNode.textContent = PROPERTY_OUTPUT_ORDER[i].header;
          headerCell.appendChild(linkNode);
        } else {
          // Output header text without a URL.
          headerCell.textContent = PROPERTY_OUTPUT_ORDER[i].header;
          headerCell.className = 'table-header';
          headerCell.title = PROPERTY_OUTPUT_ORDER[i].tooltip;
        }
        row.appendChild(headerCell);
      }
    }
    return row;
  }

  /**
   * @param {Object} autocompleteSuggestion the particular autocomplete
   *     suggestion we're in the process of displaying.
   * @param {string} propertyName the particular property of the autocomplete
   *     suggestion that should go in this cell.
   * @return {HTMLTableCellElement} that contains the value within this
   *     autocompleteSuggestion associated with propertyName.
   */
  function createCellForPropertyAndRemoveProperty(autocompleteSuggestion,
                                                  propertyName) {
    var cell = document.createElement('td');
    if (propertyName in autocompleteSuggestion) {
      if (propertyName == 'additional_info') {
        // |additional_info| embeds a two-column table of provider-specific data
        // within this cell.
        var additionalInfoTable = document.createElement('table');
        for (var additionalInfoKey in autocompleteSuggestion[propertyName]) {
          var additionalInfoRow = document.createElement('tr');

          // Set the title (name of property) cell text.
          var propertyCell = document.createElement('td');
          propertyCell.textContent = additionalInfoKey + ':';
          propertyCell.className = 'additional-info-property';
          additionalInfoRow.appendChild(propertyCell);

          // Set the value of the property cell text.
          var valueCell = document.createElement('td');
          valueCell.textContent =
              autocompleteSuggestion[propertyName][additionalInfoKey];
          valueCell.className = 'additional-info-value';
          additionalInfoRow.appendChild(valueCell);

          additionalInfoTable.appendChild(additionalInfoRow);
        }
        cell.appendChild(additionalInfoTable);
      } else if (typeof autocompleteSuggestion[propertyName] == 'boolean') {
        // If this is a boolean, display a checkmark or an X instead of
        // the strings true or false.
        if (autocompleteSuggestion[propertyName]) {
          cell.className = 'check-mark';
          cell.textContent = '✔';
        } else {
          cell.className = 'x-mark';
          cell.textContent = '✗';
        }
      } else {
        var text = String(autocompleteSuggestion[propertyName]);
        // If it's a URL wrap it in an href.
        var re = /^(http|https|ftp|chrome|file):\/\//;
        if (re.test(text)) {
          var aCell = document.createElement('a');
          aCell.textContent = text;
          aCell.href = text;
          cell.appendChild(aCell);
        } else {
          // All other data types (integer, strings, etc.) display their
          // normal toString() output.
          cell.textContent = autocompleteSuggestion[propertyName];
        }
      }
    }  // else: if propertyName is undefined, we leave the cell blank
    return cell;
  }

  /**
   * Called by C++ code when we get an update from the
   * AutocompleteController.  We simply append the result to
   * progressiveAutocompleteResults and refresh the page.
   */
  function handleNewAutocompleteResult(result) {
    progressiveAutocompleteResults.push(result);
    refresh();
  }

  /**
   * Appends some human-readable information about the provided
   * autocomplete result to the HTML node with id omnibox-debug-text.
   * The current human-readable form is a few lines about general
   * autocomplete result statistics followed by a table with one line
   * for each autocomplete match.  The input parameter result is a
   * complex Object with lots of information about various
   * autocomplete matches.  Here's an example of what it looks like:
   * <pre>
   * {@code
   * {
   *   'done': false,
   *   'time_since_omnibox_started_ms': 15,
   *   'combined_results' : {
   *     'num_items': 4,
   *     'item_0': {
   *       'destination_url': 'http://mail.google.com',
   *       'provider_name': 'HistoryURL',
   *       'relevance': 1410,
   *       ...
   *     }
   *     'item_1: {
   *       ...
   *     }
   *     ...
   *   }
   *   'results_by_provider': {
   *     'HistoryURL' : {
   *       'num_items': 3,
   *         ...
   *       }
   *     'Search' : {
   *       'num_items': 1,
   *       ...
   *     }
   *     ...
   *   }
   * }
   * }
   * </pre>
   * For more information on how the result is packed, see the
   * corresponding code in chrome/browser/ui/webui/omnibox_ui.cc
   */
  function addResultToOutput(result) {
    var output = $('omnibox-debug-text');
    var inDetailedMode = $('show-details').checked;
    var showIncompleteResults = $('show-incomplete-results').checked;
    var showPerProviderResults = $('show-all-providers').checked;

    // Always output cursor position.
    var p = document.createElement('p');
    p.textContent = 'cursor position = ' + cursorPositionUsed;
    output.appendChild(p);

    // Output the result-level features in detailed mode and in
    // show incomplete results mode.  We do the latter because without
    // these result-level features, one can't make sense of each
    // batch of results.
    if (inDetailedMode || showIncompleteResults) {
      var p1 = document.createElement('p');
      p1.textContent = 'elapsed time = ' +
          result.time_since_omnibox_started_ms + 'ms';
      output.appendChild(p1);
      var p2 = document.createElement('p');
      p2.textContent = 'all providers done = ' + result.done;
      output.appendChild(p2);
    }

    // Combined results go after the lines below.
    var group = document.createElement('a');
    group.className = 'group-separator';
    group.textContent = 'Combined results.';
    output.appendChild(group);

    // Add combined/merged result table.
    var p = document.createElement('p');
    p.appendChild(addResultTableToOutput(result.combined_results));
    output.appendChild(p);

    // Move forward only if you want to display per provider results.
    if (!showPerProviderResults) {
      return;
    }

    // Individual results go after the lines below.
    var group = document.createElement('a');
    group.className = 'group-separator';
    group.textContent = 'Results for individual providers.';
    output.appendChild(group);

    // Add the per-provider result tables with labels. We do not append the
    // combined/merged result table since we already have the per provider
    // results.
    for (var provider in result.results_by_provider) {
      var results = result.results_by_provider[provider];
      // If we have no results we do not display anything.
      if (results.num_items == 0) {
        continue;
      }
      var p = document.createElement('p');
      p.appendChild(addResultTableToOutput(results));
      output.appendChild(p);
    }
  }

  /**
   * @param {Object} result either the combined_results component of
   *     the structure described in the comment by addResultToOutput()
   *     above or one of the per-provider results in the structure.
   *     (Both have the same format).
   * @return {HTMLTableCellElement} that is a user-readable HTML
   *     representation of this object.
   */
  function addResultTableToOutput(result) {
    var inDetailedMode = $('show-details').checked;
    // Create a table to hold all the autocomplete items.
    var table = document.createElement('table');
    table.className = 'autocomplete-results-table';
    table.appendChild(createAutocompleteResultTableHeader());
    // Loop over every autocomplete item and add it as a row in the table.
    for (var i = 0; i < result.num_items; i++) {
      var autocompleteSuggestion = result['item_' + i];
      var row = document.createElement('tr');
      // Loop over all the columns/properties and output either them
      // all (if we're in detailed mode) or only the ones marked displayAlways.
      // Keep track of which properties we displayed.
      var displayedProperties = {};
      for (var j = 0; j < PROPERTY_OUTPUT_ORDER.length; j++) {
        if (inDetailedMode || PROPERTY_OUTPUT_ORDER[j].displayAlways) {
          row.appendChild(createCellForPropertyAndRemoveProperty(
              autocompleteSuggestion, PROPERTY_OUTPUT_ORDER[j].propertyName));
          displayedProperties[PROPERTY_OUTPUT_ORDER[j].propertyName] = true;
        }
      }

      // Now, if we're in detailed mode, add all the properties that
      // haven't already been output.  (We know which properties have
      // already been output because we delete the property when we output
      // it.  The only way we have properties left at this point if
      // we're in detailed mode and we're getting back properties
      // not listed in PROPERTY_OUTPUT_ORDER.  Perhaps someone added
      // something to the C++ code but didn't bother to update this
      // Javascript?  In any case, we want to display them.)
      if (inDetailedMode) {
        for (var key in autocompleteSuggestion) {
          if (!displayedProperties[key]) {
            var cell = document.createElement('td');
            cell.textContent = key + '=' + autocompleteSuggestion[key];
            row.appendChild(cell);
          }
        }
      }

      table.appendChild(row);
    }
    return table;
  }

  /* Repaints the page based on the contents of the array
   * progressiveAutocompleteResults, which represents consecutive
   * autocomplete results.  We only display the last (most recent)
   * entry unless we're asked to display incomplete results.  For an
   * example of the output, play with chrome://omnibox/
   */
  function refresh() {
    // Erase whatever is currently being displayed.
    var output = $('omnibox-debug-text');
    output.innerHTML = '';

    if (progressiveAutocompleteResults.length > 0) {  // if we have results
      // Display the results.
      var showIncompleteResults = $('show-incomplete-results').checked;
      var startIndex = showIncompleteResults ? 0 :
          progressiveAutocompleteResults.length - 1;
      for (var i = startIndex; i < progressiveAutocompleteResults.length; i++) {
        addResultToOutput(progressiveAutocompleteResults[i]);
      }
    }
  }

  return {
    initialize: initialize,
    startOmniboxQuery: startOmniboxQuery,
    handleNewAutocompleteResult: handleNewAutocompleteResult
  };
});

document.addEventListener('DOMContentLoaded', omniboxDebug.initialize);
