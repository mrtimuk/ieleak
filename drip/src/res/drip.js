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
	};
}

function __drip_onPropertyChange() {
	if (window.event.propertyName == 'innerHTML') {
		__drip_jsHook.logNode(window.event.srcElement, document, true);
	}
}

function __drip_onFunctionCall(obj, functionName, returnValue) {
	switch (functionName)
	{
	case 'cloneNode':
	case 'appendChild':
	case 'insertBefore':
		__drip_jsHook.logNode(returnValue, document, true);
		break;

	case 'insertAdjacentElement':
		__drip_jsHook.logNode(returnValue.parentNode || returnValue, document, true);
		break;

	case 'insertAdjacentHTML':
		__drip_jsHook.logNode(obj.parentNode || obj, document, true);
		break;

	default:
		break;
	}
}

/* This function attaches to a native function and triggers a notification when it gets called.
 * NOTE: This function allows only a limited number of parameters.
 */
function __drip_createOverrideFunction(functionName, nativeFunction) {
	return function(arg1, arg2, arg3) {
		/* Because of the self-altering nature of the overridden functions, the value
		 * of "this" must be preserved for the callback notification
		 */
		var self = this;

		/* simulate Function.call */
		this.__drip_call = nativeFunction;
		var result = this.__drip_call(arg1, arg2, arg3);
		this.__drip_call = void 0;

		__drip_onFunctionCall(self, functionName, result);
		return result;
	}
}

function __drip_hookEvents(elem) {
	/* NOTE: don't double-register functions */
	if (elem.__drip_hooked) return;

	if (typeof elem.attachEvent !== 'undefined')
		elem.attachEvent('onpropertychange', __drip_onPropertyChange);

	/* Element references might change when an element is attached to the document */
	var functionNames = ['cloneNode','appendChild','insertBefore','insertAdjacentElement','insertAdjacentHTML'];
	for (var i = 0; i < functionNames.length; i++) {
		var name = functionNames[i];

		// the XML island getter throws an exception
		var nativeFunction;
		try { nativeFunction = elem[name]; } catch (err) { }

		if (typeof nativeFunction !== 'undefined') {
			var override = __drip_createOverrideFunction(name, nativeFunction);
			/* An exception may be thrown if properties cannot be set on the element. */
			try { elem[name] = override; } catch (err)	{ }
		}
	}

	/* An exception may be thrown if properties cannot be set on the element. */
	try { elem.__drip_hooked = true; } catch(err) { }
}
