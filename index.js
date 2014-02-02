HelloTutorialModule = null;  // Global application object.
var statusText = 'NO-STATUS';

function updateStatus(opt_message) {
    if (opt_message)
        statusText = opt_message;
    var statusField = document.getElementById('statusField');
    if (statusField) {
        statusField.innerHTML = statusText;
    }
}

var onPluginLoaded = function() {
    HelloTutorialModule = document.getElementById('naclPlugin');
    updateStatus('Loaded');
}

function errorHandler(err) {
    console.log(err);
}

function onPluginMessage(message_event) {
    if (message_event.data instanceof ArrayBuffer) {
	console.log("got PDF i assume");

	chrome.fileSystem.chooseEntry({'type': 'saveFile'}, function(entry) {
	    entry.createWriter(function(fileWriter) {
		fileWriter.write(new Blob([new Int8Array(message_event.data)], {type: 'application/x-pdf'}));
	    }, errorHandler);
	});

	return;
    }
    if (message_event.data instanceof String) {
	console.log("got string: " + message_event.data);
	return;
    }
    console.log(message_event.data);
}

function openPDF() {
    chrome.fileSystem.chooseEntry({'type': 'openFile'}, function(entry, fEntries) {
	entry.file(function(file) {
	    var reader = new FileReader();
	    reader.onload = function(info) {
		console.log('read completed ' + info.target.result.byteLength);
		HelloTutorialModule.postMessage(info.target.result);
	    }
	    reader.readAsArrayBuffer(file);
	});
    });
}

function exportPDF() {
    HelloTutorialModule.postMessage('exportPDF');
}

gZoom = 1.0;

function zoomIn() {
    gZoom *= 1.3;
    HelloTutorialModule.postMessage('zoomTo:' + gZoom);
}

function zoomOut() {
    gZoom /= 1.3;
    HelloTutorialModule.postMessage('zoomTo:' + gZoom);
}

function onProgressMessage(event) {
    var loadPercent = 0.0;
    var loadPercentString;
    var elt = document.getElementById('statusField');
    if (event.lengthComputable && event.total > 0) {
	loadPercent = (event.loaded / event.total * 100.0) | 0;
	loadPercentString = loadPercent + '%';
	elt.innerText = loadPercentString;
    } else {
    // The total length is not yet known.
	elt.innerText = 'Loading';
    }
}

function selectToolArrow() {
    HelloTutorialModule.postMessage('selectTool:Arrow');
}
function selectToolText() {
    HelloTutorialModule.postMessage('selectTool:Text');
}
function selectToolCircle() {
    HelloTutorialModule.postMessage('selectTool:Circle');
}
function selectToolRectangle() {
    HelloTutorialModule.postMessage('selectTool:Rectangle');
}
function selectToolSquiggle() {
    HelloTutorialModule.postMessage('selectTool:Squiggle');
}
function selectToolCheckmark() {
    HelloTutorialModule.postMessage('selectTool:Checkmark');
}

window.onload = function() {
    console.log(chrome.runtime.getURL('datafile.txt'));

    var plugin = window.document.createElement('embed');
    HelloTutorialModule = plugin;
    plugin.addEventListener('load', onPluginLoaded);

    plugin.id = 'naclPlugin';
    plugin.setAttribute('src', 'pdfsketch.nmf');
    plugin.setAttribute('type', 'application/x-pnacl');
    plugin.addEventListener('progress', onProgressMessage);
    plugin.addEventListener('message', onPluginMessage);
    plugin.addEventListener('crash', function (ev) {
	console.log('plugin crashed');
    });

    var parentDiv = document.getElementById('naclContainer');
    parentDiv.insertBefore(plugin, parentDiv.firstChild);

    document.getElementById('buttonOpen').onclick = openPDF;
    document.getElementById('buttonExportPDF').onclick = exportPDF;
    document.getElementById('buttonZoomIn').onclick = zoomIn;
    document.getElementById('buttonZoomOut').onclick = zoomOut;

    document.getElementById('buttonToolArrow').onclick = selectToolArrow;
    document.getElementById('buttonToolText').onclick = selectToolText;
    document.getElementById('buttonToolCircle').onclick = selectToolCircle;
    document.getElementById('buttonToolRectangle').onclick = selectToolRectangle;
    document.getElementById('buttonToolSquiggle').onclick = selectToolSquiggle;
    document.getElementById('buttonToolCheckmark').onclick = selectToolCheckmark;
}
