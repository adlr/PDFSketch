// Copyright...

// Callback is called like: callback([r, g, b, alpha])

function ColorPicker(label, callback) {
  this.callback = callback;
  this.initDivs_(label);
  this.currentColor = [0.0, 0.0, 0.0, 1.0];
  this.popupVisible = false;
}

ColorPicker.prototype.colorClicked_ = function(color) {
  this.hide();
  if (color) {
    for (var i = 0; i < 3; i++) {
      this.currentColor[i] = color[i];
    }
  }
  this.updateChip_();
  this.callback(this.currentColor);
}

ColorPicker.prototype.updateChip_ = function() {
  this.colorChip.style.backgroundColor = 'rgba(' + this.currentColor.join(',') + ')';
  this.opacityInput.value = parseInt(this.currentColor[3]);
}

ColorPicker.prototype.opacityTextChanged_ = function(evt) {
  var num = parseFloat(this.opacityInput.value);
  console.log('num: ' + num);
  if (num >= 0.0 && num <= 100.0)
    this.currentColor[3] = num / 100.0;
  this.updateChip_();
  if (evt && evt.keyIdentifier && evt.keyIdentifier == 'Enter')
    this.colorClicked_(null);
}

ColorPicker.prototype.initDivs_ = function(label) {
  // Create button
  this.button = document.createElement('button');
  this.button.className = 'colorButton';
  this.button.appendChild(document.createTextNode(label));
  var colorChipBackground = document.createElement('div');
  colorChipBackground.className = 'colorChipBackground';
  this.button.appendChild(colorChipBackground);
  this.colorChip = document.createElement('div');
  this.colorChip.className = 'colorChip';
  this.button.appendChild(this.colorChip);
  var self = this;
  this.button.onclick = function() {
    self.toggle();
  };

  // Create drop-down menu
  this.popupDiv = document.createElement('div');
  this.popupDiv.className = 'colorPopup';
  this.popupDiv.style.display = 'flex';
  this.popupDiv.style.flexDirection = 'column';
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
      if (newColor == this.currentColor) {
	colorDiv.className += ' colorPopupCellSelected';
      }

      (function(newColorCopy) {
	colorDiv.onclick = function() {
	  self.colorClicked_(newColorCopy);
	}
      })(color.color);

      rowDiv.appendChild(colorDiv);
    }
    this.popupDiv.appendChild(rowDiv);
  }
  var opacityDiv = document.createElement('div');
  opacityDiv.appendChild(document.createTextNode('Opacity: '));
  this.opacityInput = document.createElement('input');
  this.opacityInput.onchange =
    this.opacityInput.onkeyup =
    this.opacityInput.onpaste = function(evt) {
      self.opacityTextChanged_(evt);
    };
  this.opacityInput.value = '100';
  this.opacityInput.style.width = '1.9em';
  this.opacityInput.style.textAlign = 'right';
  opacityDiv.appendChild(this.opacityInput);
  opacityDiv.appendChild(document.createTextNode('%'));
  this.popupDiv.appendChild(opacityDiv);
}

ColorPicker.prototype.getButtonDiv = function() {
  return this.button;
}

ColorPicker.prototype.toggle = function() {
  if (!this.popupVisible) {
    this.show();
  } else {
    this.hide();
  }
}

ColorPicker.prototype.show = function() {
  var pos = this.button.getBoundingClientRect();
  this.popupDiv.style.position = 'absolute';
  this.popupDiv.style.left = pos.left + 'px';
  this.popupDiv.style.top = pos.bottom + 'px';
  this.popupDiv.style.visibility = 'visible';
  document.body.appendChild(this.popupDiv);
  this.popupVisible = true;
};

ColorPicker.prototype.hide = function() {
  this.popupDiv.remove();
  this.popupVisible = false;
}

ColorPicker.prototype.setColor = function(color) {
  for (var i = 0; i < 4; i++) {
    this.currentColor[i] = color[i];
  }
  this.updateChip_();
};

