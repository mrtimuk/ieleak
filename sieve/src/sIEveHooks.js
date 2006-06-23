function __sIEve_initializeHooks(jsHook)
{
	window.attachEvent('onbeforeunload',function ()
	{
		jsHook.rescanForElements(window.document);
		window.detachEvent('onbeforeunload',arguments.callee);
	});

	window.attachEvent('onunload',function ()
	{
		try {
			jsHook.unloadWindow(window.document);
			if ( window.__sIEve_rescanForElementsTimer )
			{
				window.clearTimeout(window.__sIEve_rescanForElementsTimer);
				window.__sIEve_rescanForElementsTimer = null;
				window.navigator.__sIEve_rescanForElementsIntervalHandler = null;
			}
			window.__sIEve_logElement = null;
			window.document.createElement = null;
			window.detachEvent('onunload',arguments.callee);
		}
		catch (err) { /* Ignore errors on close of sIEve */ }
	});

	window.document.attachEvent('onstop',function ()
	{
		try
		{
			jsHook.unloadWindow(window.document);
			if ( window.__sIEve_rescanForElementsTimer )
			{
				window.clearTimeout(window.__sIEve_rescanForElementsTimer);
				window.__sIEve_rescanForElementsTimer = null;
				window.navigator.__sIEve_rescanForElementsIntervalHandler = null;
			}
			window.__sIEve_logElement = null;
			window.document.createElement = null;
			window.document.detachEvent('onstop',arguments.callee);
		}
		catch (err) { /* Ignore errors on close of sIEve */ }
	});

	if ( window.document.location.href.indexOf('blank.htm') < 0 )
	{
		window.navigator.__sIEve_rescanForElementsInterval = 50;
	}
	if ( ! window.navigator.__sIEve_rescanForElementsIntervalHandler )
	{
		window.navigator.__sIEve_rescanForElementsIntervalHandler = function()
		{
			jsHook.rescanForElements();
			window.__sIEve_setRescanForElementsTimeout();
		};
		window.__sIEve_setRescanForElementsTimeout();
	}
	else
	{
		if ( !window.library ) window.setTimeout(function(){jsHook.rescanForElements(window.document);},100);
	}

	window.__sIEve_logElement = function(elem)
	{
		jsHook.logElement(elem);
		__sIEve_overloadCloneNode(elem);
		var child = elem.firstChild;
		while ( child ) {
			__sIEve_logElement(child);
			child = child.nextSibling;
		}
	};

	var nativeCreateElement = window.document.createElement;
	window.document.createElement = function(tag) {
		var elem = nativeCreateElement(tag);
		__sIEve_logElement(elem);
		return elem;
	};
}

function __sIEve_setRescanForElementsTimeout()
{
	window.navigator.__sIEve_rescanForElementsInterval *= 2;
	if ( window.navigator.__sIEve_rescanForElementsInterval >= 5000 )
	{
		window.navigator.__sIEve_rescanForElementsInterval = 5000;
	}
	window.__sIEve_rescanForElementsTimer = window.setTimeout(window.navigator.__sIEve_rescanForElementsIntervalHandler,window.navigator.__sIEve_rescanForElementsInterval);
}

function __sIEve_overloadCloneNode(elem)
{
	if (elem.nodeType == 1 && !elem.__sIEve_nativeCloneNode)
	{
		try
		{
			elem.__sIEve_nativeCloneNode = elem.cloneNode;
			elem.cloneNode = __sIEve_customCloneNode;
		}
		catch ( err ) { /* Some elements doesn't allow override e.g. <EMBED> */ }
	}
}

function __sIEve_customCloneNode(deep)
{
	var clone = this.__sIEve_nativeCloneNode(deep);
	if ( clone )
	{
		window.__sIEve_logElement(clone);
	}
	return clone;
}
