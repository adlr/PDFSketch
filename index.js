function stringStartsWith(str, prefix) {
    return str.slice(0, prefix.length) == prefix;
};

HelloTutorialModule = null;  // Global application object.
var statusText = 'NO-STATUS';
var gOpenFileEntry = null;

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

function isPDFSketch(array_buf) {
    var intarr = new Int8Array(array_buf);
    return intarr[0] == 's'.charCodeAt(0) &&
	intarr[1] == 'k'.charCodeAt(0) &&
	intarr[2] == 'c'.charCodeAt(0) &&
	intarr[3] == 'h'.charCodeAt(0);
}

function onPluginMessage(message_event) {
    if (message_event.data instanceof ArrayBuffer) {
	if (isPDFSketch(message_event.data)) {
	    handlePDFSketchSave(message_event.data);
	    return;
	}
	console.log("got PDF i assume");

	if (gOpenFileEntry) {
	    // Save to existing file
	    gOpenFileEntry.createWriter(function(fileWriter) {
		fileWriter.onwriteend = function(evt) {
		    fileWriter.onwriteend = function(evt) {
			console.log('saved');
			document.getElementById('statusField').innerText = 'Saved';
		    };
		    fileWriter.write(new Blob([new Int8Array(message_event.data)], {type: 'application/x-pdf'}));
		}
		fileWriter.seek(0);
		fileWriter.truncate(0);
	    }, errorHandler);
	    return;
	}

	chrome.fileSystem.chooseEntry({'type': 'saveFile'}, function(entry) {
	    entry.createWriter(function(fileWriter) {
		fileWriter.onwriteend = function(evt) {
		    fileWriter.onwriteend = function(evt) { console.log('saved'); };
		    fileWriter.write(new Blob([new Int8Array(message_event.data)], {type: 'application/x-pdf'}));
		}
		fileWriter.seek(0);
		fileWriter.truncate(0);
	    }, errorHandler);
	});

	return;
    }
    if (typeof(message_event.data) === 'string') {
	var TOOL_SELECTED_PREFIX = 'ToolSelected:';
	if (stringStartsWith(message_event.data, TOOL_SELECTED_PREFIX)) {
	    toolSelected(message_event.data.slice(TOOL_SELECTED_PREFIX.length));
	}
	var UNDO_ENABLED_PREFIX = 'undoEnabled:';
	if (stringStartsWith(message_event.data, UNDO_ENABLED_PREFIX)) {
	    setUndoEnabled(message_event.data.slice(UNDO_ENABLED_PREFIX.length));
	}
	var REDO_ENABLED_PREFIX = 'redoEnabled:';
	if (stringStartsWith(message_event.data, REDO_ENABLED_PREFIX)) {
	    setRedoEnabled(message_event.data.slice(REDO_ENABLED_PREFIX.length));
	}
	// console.log(message_event.data);
	return;
    }
    console.log(message_event.data);
}

function openPDF() {
    chrome.fileSystem.chooseEntry({'type': 'openFile'}, function(entry, fEntries) {
	gOpenFileEntry = entry;
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

gSaveFileEntry = null;

function save() {
    HelloTutorialModule.postMessage('save');
}

function saveAs() {
    gSaveFileEntry = null;
    HelloTutorialModule.postMessage('save');
}

function handlePDFSketchSave(arraybuf) {
    var write = function() {
	gSaveFileEntry.createWriter(function(fileWriter) {
	    fileWriter.onwriteend = function(evt) {
		fileWriter.onwriteend = function(evt) { console.log('saved'); };
		fileWriter.write(new Blob([new Int8Array(arraybuf)], {type: 'application/x-pdfsketch'}));
	    }
	    fileWriter.seek(0);
	    fileWriter.truncate(0);
	}, errorHandler);
    };

    if (!gSaveFileEntry) {
	chrome.fileSystem.chooseEntry(
	    {type: 'saveFile',
	     accepts: [{ extensions: ['html']}]
	    }, function(entry) {
		gSaveFileEntry = entry;
		write();
	    });
    } else {
	write();
    }
}

function exportPDF() {
    document.getElementById('statusField').innerText = 'Saving...';
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

function undo() {
    HelloTutorialModule.postMessage('undo');
}
function redo() {
    HelloTutorialModule.postMessage('redo');
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

function toolSelected(tool) {
    var buttons = {
	'Arrow': document.getElementById('buttonToolArrow'),
	'Text': document.getElementById('buttonToolText'),
	'Circle': document.getElementById('buttonToolCircle'),
	'Rectangle': document.getElementById('buttonToolRectangle'),
	'Squiggle': document.getElementById('buttonToolSquiggle'),
	'Checkmark': document.getElementById('buttonToolCheckmark')
    };
    for (type in buttons) {
	buttons[type].className = '';
    }
    buttons[tool].className = 'selectedButton';
}

function setUndoEnabled(enabled) {
    document.getElementById('buttonUndo').disabled = enabled != 'true';
}
function setRedoEnabled(enabled) {
    document.getElementById('buttonRedo').disabled = enabled != 'true';
}

window.onload = function() {
    //console.log(chrome.runtime.getURL('datafile.txt'));

    var plugin = window.document.createElement('embed');
    HelloTutorialModule = plugin;
    plugin.addEventListener('load', onPluginLoaded);

    plugin.id = 'naclPlugin';
    if (false) {  // pnacl
	plugin.setAttribute('src', 'pdfsketch.nmf');
	plugin.setAttribute('type', 'application/x-pnacl');
    } else {
	plugin.setAttribute('src', 'pdfsketch_nexe.nmf');
	plugin.setAttribute('type', 'application/x-nacl');
    }
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
    document.getElementById('buttonUndo').onclick = undo;
    document.getElementById('buttonRedo').onclick = redo;

    document.getElementById('buttonToolArrow').onclick = selectToolArrow;
    document.getElementById('buttonToolText').onclick = selectToolText;
    document.getElementById('buttonToolCircle').onclick = selectToolCircle;
    document.getElementById('buttonToolRectangle').onclick = selectToolRectangle;
    document.getElementById('buttonToolSquiggle').onclick = selectToolSquiggle;
    document.getElementById('buttonToolCheckmark').onclick = selectToolCheckmark;

    var githubLink = document.getElementById('githublink');
    githubLink.onclick = function() {
	window.open('https://github.com/adlr/PDFSketch');
    }
}
