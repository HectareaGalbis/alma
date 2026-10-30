// Pages of the documentation, in order. The sidebar and the previous and next links are built from
// this list, so a new page only needs to be added here.
const PAGES = [
    { file: "index.html", title: "Inicio" },
    { file: "empezar.html", title: "Primeros pasos" },
    { file: "c.html", title: "El lenguaje C" },
    { file: "generar-c.html", title: "Generar C" },
    { file: "lenguaje.html", title: "ALMA al compilar" },
    { file: "procedimientos.html", title: "Funciones y macros" },
    { file: "datos.html", title: "Listas, vectores y tablas hash" },
    { file: "packages.html", title: "Packages y macro characters" },
    { file: "proyecto.html", title: "Sobre el proyecto" },
];

function currentFile() {
    const name = location.pathname.split("/").pop();
    return name === "" ? "index.html" : name;
}

function slugify(text) {
    return text
        .toLowerCase()
        .normalize("NFD")
        .replace(/[̀-ͯ]/g, "")
        .replace(/[^a-z0-9]+/g, "-")
        .replace(/^-|-$/g, "");
}

function headings() {
    const used = new Set();
    const list = [];
    for (const h of document.querySelectorAll(".content h2, .content h3")) {
        if (!h.id) {
            let id = slugify(h.textContent);
            for (let i = 2; used.has(id); i++)
                id = slugify(h.textContent) + "-" + i;
            h.id = id;
        }
        used.add(h.id);
        const anchor = document.createElement("a");
        anchor.className = "anchor";
        anchor.href = "#" + h.id;
        anchor.textContent = "#";
        anchor.setAttribute("aria-hidden", "true");
        h.appendChild(anchor);
        list.push(h);
    }
    return list;
}

function headingText(h) {
    return Array.from(h.childNodes)
        .filter((n) => !(n.classList && n.classList.contains("anchor")))
        .map((n) => n.textContent)
        .join("")
        .trim();
}

function buildSidebar(sections) {
    const sidebar = document.getElementById("sidebar");
    if (!sidebar)
        return;
    const file = currentFile();
    const ul = document.createElement("ul");
    for (const page of PAGES) {
        const li = document.createElement("li");
        const a = document.createElement("a");
        a.href = page.file;
        a.textContent = page.title;
        li.appendChild(a);
        if (page.file === file) {
            a.classList.add("current");
            const h2s = sections.filter((h) => h.tagName === "H2");
            if (h2s.length > 0) {
                const sub = document.createElement("ul");
                for (const h of h2s) {
                    const sli = document.createElement("li");
                    const sa = document.createElement("a");
                    sa.href = "#" + h.id;
                    sa.textContent = headingText(h);
                    sa.addEventListener("click", () => document.body.classList.remove("menu-open"));
                    sli.appendChild(sa);
                    sub.appendChild(sli);
                }
                li.appendChild(sub);
            }
        }
        ul.appendChild(li);
    }
    sidebar.appendChild(ul);
}

function buildToc(sections) {
    const toc = document.getElementById("toc");
    if (!toc || sections.length === 0)
        return;
    const title = document.createElement("h2");
    title.textContent = "En esta página";
    toc.appendChild(title);
    const ul = document.createElement("ul");
    const links = [];
    for (const h of sections) {
        const li = document.createElement("li");
        const a = document.createElement("a");
        a.href = "#" + h.id;
        a.textContent = headingText(h);
        a.className = h.tagName === "H3" ? "level-3" : "level-2";
        li.appendChild(a);
        ul.appendChild(li);
        links.push(a);
    }
    toc.appendChild(ul);

    const update = () => {
        let active = 0;
        for (let i = 0; i < sections.length; i++)
            if (sections[i].getBoundingClientRect().top < 100)
                active = i;
        links.forEach((a, i) => a.classList.toggle("active", i === active));
    };
    document.addEventListener("scroll", update, { passive: true });
    update();
}

function buildPager() {
    const pager = document.getElementById("pager");
    if (!pager)
        return;
    const index = PAGES.findIndex((p) => p.file === currentFile());
    if (index < 0)
        return;
    const link = (page, label, cls) => {
        const a = document.createElement("a");
        a.href = page.file;
        a.className = cls;
        const small = document.createElement("small");
        small.textContent = label;
        a.appendChild(small);
        a.appendChild(document.createTextNode(page.title));
        return a;
    };
    if (index > 0)
        pager.appendChild(link(PAGES[index - 1], "Anterior", "prev"));
    if (index + 1 < PAGES.length)
        pager.appendChild(link(PAGES[index + 1], "Siguiente", "next"));
}

// A small highlighter for the code of ALMA: comments, strings, characters, keywords, numbers and
// the first element of each list.
function escapeHtml(text) {
    return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function highlightAlma(code) {
    const delimiter = /[\s()'`,";]/;
    let out = "";
    let i = 0;
    let afterParen = false;
    const span = (cls, text) => '<span class="' + cls + '">' + escapeHtml(text) + "</span>";
    while (i < code.length) {
        const c = code[i];
        if (c === ";") {
            let j = code.indexOf("\n", i);
            if (j < 0)
                j = code.length;
            out += span("tok-comment", code.slice(i, j));
            i = j;
            afterParen = false;
        } else if (c === '"') {
            let j = i + 1;
            while (j < code.length && code[j] !== '"')
                j += code[j] === "\\" ? 2 : 1;
            j = Math.min(j + 1, code.length);
            out += span("tok-string", code.slice(i, j));
            i = j;
            afterParen = false;
        } else if (c === "\\") {
            let j = i + 2;
            while (j < code.length && /[a-zA-Z]/.test(code[j]) && /[a-zA-Z]/.test(code[i + 1]))
                j++;
            out += span("tok-char", code.slice(i, j));
            i = j;
            afterParen = false;
        } else if (c === "(" || c === "#" && code[i + 1] === "(") {
            const j = c === "#" ? i + 2 : i + 1;
            out += escapeHtml(code.slice(i, j));
            i = j;
            afterParen = c === "(";
        } else if (/\s/.test(c) || /[)'`,]/.test(c)) {
            out += escapeHtml(c);
            i++;
            if (!/\s/.test(c))
                afterParen = false;
        } else {
            let j = i;
            while (j < code.length && !delimiter.test(code[j]))
                j++;
            if (j === i)
                j = i + 1;
            const token = code.slice(i, j);
            if (token.startsWith(":"))
                out += span("tok-keyword", token);
            else if (/^[+-]?\d+$/.test(token))
                out += span("tok-number", token);
            else if (afterParen)
                out += span("tok-head", token);
            else
                out += escapeHtml(token);
            i = j;
            afterParen = false;
        }
    }
    return out;
}

function highlight() {
    for (const block of document.querySelectorAll("pre code.lisp"))
        block.innerHTML = highlightAlma(block.textContent);
}

function setupMenu() {
    const button = document.querySelector(".menu-button");
    if (button)
        button.addEventListener("click", () => document.body.classList.toggle("menu-open"));
}

document.addEventListener("DOMContentLoaded", () => {
    const sections = headings();
    buildSidebar(sections);
    buildToc(sections);
    buildPager();
    highlight();
    setupMenu();
});
