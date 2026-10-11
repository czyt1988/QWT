/*
    MathJax configuration for the QWT7 documentation.

    The Markdown sources use the CSDN / GitHub compatible delimiters
    $ ... $ ( inline ) and $$ ... $$ ( block ).  pymdownx.arithmatex runs
    in "generic" mode: it wraps the TeX source in elements that carry the
    "arithmatex" class and normalizes the delimiters to \( ... \) and
    \[ ... \] in the HTML, which MathJax then typesets.
*/

window.MathJax = {
    tex: {
        inlineMath: [["\\(", "\\)"]],
        displayMath: [["\\[", "\\]"]],
        processEscapes: true,
        processEnvironments: true
    },
    options: {
        ignoreHtmlClass: ".*|",
        processHtmlClass: "arithmatex"
    }
};

(function () {
    "use strict";

    function typeset() {
        if (!window.MathJax || !MathJax.startup) {
            return;
        }

        MathJax.startup.output.clearCache();
        MathJax.typesetClear();
        MathJax.texReset();
        MathJax.typesetPromise();
    }

    /* Material provides the document$ observable, that also fires when a
       page has been swapped in by instant loading. Without it ( instant
       loading is disabled in this project ) the page is typeset once the
       document has been parsed - the MathJax bundle is a synchronous
       script, so it has been executed at that point. */
    if (typeof document$ !== "undefined" && document$.subscribe) {
        document$.subscribe(typeset);
    } else if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", typeset);
    } else {
        typeset();
    }
})();
