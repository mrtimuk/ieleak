var __drip_jsHook;
function __drip_initHook(jsHook) {
  __drip_jsHook = jsHook;

  var oldCE = document.createElement;
  document.createElement = function(tag) {
    var elem = oldCE(tag);
    jsHook.logNode(elem, document, false);
    return elem;
  };
  var oldCDF = document.createDocumentFragment;
  document.createDocumentFragment = function() {
    var elem = oldCDF();
    __drip_hookEvents(elem);
    return elem;
  }
}

function __drip_onPropertyChange() {
  if (window.event.propertyName == 'innerHTML') {
    __drip_jsHook.logNode(window.event.srcElement, document, true);
  }
}

function __drip_cloneNode(child) {
	var elem = this.__drip_native_cloneNode(child);
	__drip_jsHook.logNode(elem, document, true);
  return elem;
}

function __drip_appendChild(child) {
	var elem = this.__drip_native_appendChild(child);
	__drip_jsHook.logNode(elem, document, true);
  return elem;
}

function __drip_insertBefore(oNewNode, oChildNode) {
	var elem = this.__drip_native_insertBefore(oNewNode, oChildNode);
	__drip_jsHook.logNode(elem, document, true);
  return elem;
}

function __drip_insertAdjacentElement(sWhere, oElement) {
	var elem = this.__drip_native_insertAdjacentElement(sWhere, oElement);
	__drip_jsHook.logNode(elem.parentNode || elem, document, true);
  return elem;
}

function __drip_insertAdjacentHTML(sWhere, sText) {
	this.__drip_native_insertAdjacentHTML(sWhere, sText);
	__drip_jsHook.logNode(this.parentNode || this, document, true);
}

function __drip_hookEvents(elem) {
  /* NOTE: don't double-register functions */
  if (elem.__drip_hooked) return;
  if (elem.nodeType != 1/*ELEMENT*/ && elem.nodeType != 11/*Document Fragment*/) return;

  elem.attachEvent('onpropertychange', __drip_onPropertyChange);

  elem.__drip_native_cloneNode = elem.cloneNode;
  elem.cloneNode = __drip_cloneNode;

  /* Element references might change when an element is attached to the document */
  elem.__drip_native_appendChild = elem.appendChild;
  elem.appendChild = __drip_appendChild;

  elem.__drip_native_insertBefore = elem.insertBefore;
  elem.insertBefore = __drip_insertBefore;

  elem.__drip_native_insertAdjacentElement = elem.insertAdjacentElement;
  elem.insertAdjacentElement = __drip_insertAdjacentElement;

  elem.__drip_native_insertAdjacentHTML = elem.insertAdjacentHTML;
  elem.insertAdjacentHTML = __drip_insertAdjacentHTML;

  elem.__drip_hooked = true;
}
