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

function copyString(str) {
    var old_elt = document.activeElement;
    var clipboardio = document.getElementById('clipboardio');
    clipboardio.value = str;
    clipboardio.select();
    document.execCommand('copy');
    old_elt.focus();
}

function paste() {
    document.execCommand('paste');
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
	var COLOR_SELECTED_PREFIX = 'ColorSelected:';
	if (stringStartsWith(message_event.data, COLOR_SELECTED_PREFIX)) {
	    colorSelected(message_event.data.slice(COLOR_SELECTED_PREFIX.length));
	}
	var UNDO_ENABLED_PREFIX = 'undoEnabled:';
	if (stringStartsWith(message_event.data, UNDO_ENABLED_PREFIX)) {
	    setUndoEnabled(message_event.data.slice(UNDO_ENABLED_PREFIX.length));
	}
	var REDO_ENABLED_PREFIX = 'redoEnabled:';
	if (stringStartsWith(message_event.data, REDO_ENABLED_PREFIX)) {
	    setRedoEnabled(message_event.data.slice(REDO_ENABLED_PREFIX.length));
	}
	var COPY_PREFIX = 'copy:';
	if (stringStartsWith(message_event.data, COPY_PREFIX)) {
	    copyString(message_event.data.slice(COPY_PREFIX.length));
	}
	if (message_event.data == 'paste') {
	    paste();
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

function insertImage() {
    chrome.fileSystem.chooseEntry({'type': 'openFile'}, function(entry, fEntries) {
	entry.file(function(file) {
	    var reader = new FileReader();
	    reader.onload = function(info) {
		console.log('image read completed ' + info.target.result.byteLength);
		var cmd = {cmd: 'insertImage', data: info.target.result};
		HelloTutorialModule.postMessage(cmd);
	    }
	    reader.readAsArrayBuffer(file);
	});
    });
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
function selectColor(n) {
    HelloTutorialModule.postMessage('selectColor:' + n);
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

var colorbuttons = [ '000000', 'FF0000', '00FF00', '0000FF', 'FFFF00'];

function colorSelected(color) {
    //console.log("selected " + color);
    for (var i = 0; i < colorbuttons.length; i++) {
	var element = document.getElementById("col" + colorbuttons[i]);
        if (color == colorbuttons[i])
	    element.style.backgroundColor= "#"+color;
	else
	    element.style.backgroundColor= "#DDDDDD";
    }
}

function setUndoEnabled(enabled) {
    document.getElementById('buttonUndo').disabled = enabled != 'true';
}
function setRedoEnabled(enabled) {
    document.getElementById('buttonRedo').disabled = enabled != 'true';
}

var gStrokeSelect = null;
var gFillSelect = null;

function rgb2hex(r,g,b) {
    // from http://stackoverflow.com/questions/57803/how-to-convert-decimal-to-hex-in-javascript
    if (g !== undefined) 
        return '#' + Number(0x1000000 + r*0x10000 + g*0x100 + b).toString(16).substring(1);
    else 
        return '#' + Number(0x1000000 + r[0]*0x10000 + r[1]*0x100 + r[2]).toString(16).substring(1);
}

function createColorSelectDiv(callback, currentColor) {
    var ret = document.createElement('div');
    ret.className = 'colorPopup';
    ret.style.display = 'flex';
    ret.style.flexDirection = 'column';
    for (rowIdx in gColors) {
	var row = gColors[rowIdx];
	var rowDiv = document.createElement('div');
	rowDiv.style.display = 'flex';
	for (colIdx in row) {
	    var color = row[colIdx];
	    var colorDiv = document.createElement('div');
	    var newColor = rgb2hex(color.color);
	    colorDiv.style.backgroundColor = rgb2hex(color.color);
	    colorDiv.className = 'colorPopupCell';
	    if (newColor == currentColor) {
		colorDiv.className += ' colorPopupCellSelected';
	    }

	    (function(newColorCopy) {
		colorDiv.onclick = function() {
		    callback(newColorCopy);
		}
	    })(newColor);

	    rowDiv.appendChild(colorDiv);
	}
	ret.appendChild(rowDiv);
    }
    var opacityDiv = document.createElement('div');
    opacityDiv.appendChild(document.createTextNode('Opacity: '));
    input = document.createElement('input');
    input.value = '100';
    input.style.width = '1.9em';
    input.style.textAlign = 'right';
    opacityDiv.appendChild(input);
    opacityDiv.appendChild(document.createTextNode('%'));
    ret.appendChild(opacityDiv);
    return ret;
}

function strokeSelected(color) {
    console.log("stroke color: " + color);
    toggleStrokeSelect();
}

function toggleStrokeSelect() {
    if (gStrokeSelect) {
	// hide div
	gStrokeSelect.remove();
	gStrokeSelect = null;
	return;
    }
    gStrokeSelect = createColorSelectDiv(strokeSelected, '#ffffff');
    var pos = document.getElementById('strokeColor').getBoundingClientRect();
    gStrokeSelect.style.position = 'absolute';
    gStrokeSelect.style.left = pos.left + 'px';
    gStrokeSelect.style.top = pos.bottom + 'px';
    gStrokeSelect.style.visibility = 'visible';
    document.body.appendChild(gStrokeSelect);
}

function insertAfter(newNode, sibling) {
    // from http://stackoverflow.com/questions/4793604/how-to-do-insert-after-in-javascript-without-using-a-library
    if (sibling.parentNode.lastChild == sibling) {
	sibling.parentNode.appendChild(newNode);
    } else {
	sibling.parentNode.insertBefore(newNode, sibling.nextSibling);
    }
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
    document.getElementById('buttonInsertImage').onclick = insertImage;
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
    document.getElementById('pasteButton').onclick = function() {
	document.execCommand('paste');
    }

    var stroke = new ColorPicker('Stroke: ', function(color) {
	console.log('setStroke(' + color + ')');
    });
    insertAfter(stroke.getButtonDiv(), document.getElementById('fontItalics'));
    var fill = new ColorPicker('Fill: ', function(color) {
	console.log('setFill(' + color + ')');
    });
    insertAfter(fill.getButtonDiv(), stroke.getButtonDiv());

    // document.getElementById('strokeColor').onclick = toggleStrokeSelect;

    document.body.onpaste = function(e) {
	var items = event.clipboardData.items;
	if (items.length == 1 && items[0].kind == 'string') {
	    items[0].getAsString(function(value) {
		HelloTutorialModule.postMessage('paste:' + value);
	    });
	    return;
	}
	console.log('onpaste called. ' + items.length + ' items');
	for (var i = 0; i < items.length; i++) {
	    console.log('kind: ' + items[i].kind +
			' type: ' + items[i].type);
	    console.log(items[i]);
	}
    }

    var fontSizes = [8, 9, 10, 11, 12, 13, 14, 18, 24, 30, 36, 48, 60, 72, 96];
    for (var index in fontSizes) {
	var option = document.createElement('option');
	option.value = option.text = fontSizes[index];
	document.getElementById('fontSizeSelect').appendChild(option);
    }

    var colspan = document.getElementById('colspan');
    for (var i = 0; i < colorbuttons.length; i++) {
    	var element = document.createElement("button");
    	element.type = "button";
    	element.id = "col" + colorbuttons[i];
    	element.name = colorbuttons[i];
    	element.value = colorbuttons[i];
    	element.style.borderColor = "#" + colorbuttons[i];
    	if (i == 0)
    	    element.style.backgroundColor = "#"+colorbuttons[i];
    	else
    	    element.style.backgroundColor = "#DDDDDD";

    	element.onclick = function() {selectColor(this.value)};
    	element.innerHTML = "&nbsp;&nbsp;&nbsp;&nbsp;"
    	colspan.appendChild(element);
    }

    var githubLink = document.getElementById('githublink');
    githubLink.onclick = function() {
	window.open('https://github.com/adlr/PDFSketch');
    }
}
