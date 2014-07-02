/**
 * Listens for the app launching then creates the window
 *
 * @see http://developer.chrome.com/apps/app.runtime.html
 * @see http://developer.chrome.com/apps/app.window.html
 */
chrome.app.runtime.onLaunched.addListener(function() {
  // Center window on screen.
  var screenWidth = screen.availWidth;
  var screenHeight = screen.availHeight;
  var width = 700;
  var height = 500;

  chrome.app.window.create('open.html', {
    'id': "PDFSketchID",
    'width': width,
    'height': height,
    'minWidth': width,
    'minHeight': height,
  });
});
