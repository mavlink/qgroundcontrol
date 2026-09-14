import assert from "node:assert/strict";
import { test } from "node:test";
import navbar from "../navbar.json" with { type: "json" };
import { parseNavigation, refreshNavigation, resolveNavigation } from "./dynamic-nav.mjs";

const remoteUrl = "https://example.com/navbar.json";
const initialNav = [{ text: "Initial", link: "/en/" }];
const cachedNav = [{ text: "Cached", link: "/en/cached.html" }];
const freshNav = [{ text: "Fresh", link: "/en/fresh.html" }];
const versionNav = [{
  text: "Version",
  preservePath: true,
  items: [{ text: "Stable", link: "https://docs.qgroundcontrol.com/Stable_V5.1/en/" }],
}];

function prepareRuntime(t, cached = null) {
  const saved = [];
  const warnings = [];
  const storage = {
    getItem(key) {
      assert.equal(key, remoteUrl);
      return cached;
    },
    setItem(key, value) {
      saved.push([key, JSON.parse(value)]);
    },
  };
  const descriptor = Object.getOwnPropertyDescriptor(globalThis, "localStorage");
  Object.defineProperty(globalThis, "localStorage", { configurable: true, value: storage });
  t.after(() => {
    if (descriptor) {
      Object.defineProperty(globalThis, "localStorage", descriptor);
    } else {
      delete globalThis.localStorage;
    }
  });
  t.mock.method(console, "warn", (...args) => warnings.push(args));
  const fetchMock = t.mock.method(globalThis, "fetch", async (url) => {
    assert.equal(url, remoteUrl);
    return Response.json({ nav: freshNav });
  });
  return { storage, saved, warnings, fetchMock };
}

test("committed navigation is valid and unchanged during SSR", () => {
  assert.equal(parseNavigation(navbar), navbar.nav);
  assert.deepEqual(resolveNavigation(navbar.nav, null), navbar.nav);
});

test("invalid remote navigation is rejected", () => {
  for (const data of [
    null, {}, { nav: [] }, { nav: [null] }, { nav: [{ text: "No link" }] },
    { nav: [{ text: "Unsafe", link: "javascript:alert(1)" }] },
    { nav: [{ text: "Empty", link: "" }] },
    { nav: [{ text: "Invalid", link: "https://[" }] },
    { nav: [{ text: "Invalid group", items: {} }] },
    { nav: [{ text: "Empty group", items: [] }] },
    { nav: [{ text: "Ambiguous", items: initialNav, link: "/en/" }] },
    { nav: [{ text: "Invalid flag", link: "/en/", preservePath: "true" }] },
  ]) {
    assert.throws(() => parseNavigation(data), /Invalid docs navigation/);
  }
});

test("version links preserve the page and hash in the same tab", () => {
  const current = new URL("https://docs.qgroundcontrol.com/master/en/guide/page.html#setup");
  const resolved = resolveNavigation(versionNav, current);
  assert.equal(resolved[0].items[0].link,
    "https://docs.qgroundcontrol.com/Stable_V5.1/en/guide/page.html#setup");
  assert.equal(resolved[0].items[0].target, "_self");
  assert.equal(versionNav[0].items[0].link, "https://docs.qgroundcontrol.com/Stable_V5.1/en/");
});

test("version links follow subsequent SPA paths and hash changes", () => {
  for (const page of ["first.html#one", "second.html#two"]) {
    const current = new URL(`https://docs.qgroundcontrol.com/master/en/${page}`);
    assert.equal(resolveNavigation(versionNav, current)[0].items[0].link,
      `https://docs.qgroundcontrol.com/Stable_V5.1/en/${page}`);
  }
});

test("version roots and directory pages retain their trailing slash", () => {
  const current = new URL("https://docs.qgroundcontrol.com/master/en/");
  assert.equal(resolveNavigation(versionNav, current)[0].items[0].link,
    "https://docs.qgroundcontrol.com/Stable_V5.1/en/");
  current.pathname = "/master/en/guide/";
  assert.equal(resolveNavigation(versionNav, current)[0].items[0].link,
    "https://docs.qgroundcontrol.com/Stable_V5.1/en/guide/");
  const rootNav = [{ text: "Root", link: "/", preservePath: true }];
  assert.equal(resolveNavigation(rootNav, new URL(current.origin))[0].link, current.origin + "/");
});

test("cross-origin previews and shallower paths keep the original links", () => {
  for (const href of [
    "http://localhost:4173/master/en/guide/page.html",
    "https://preview.example.com/master/en/guide/page.html",
    "https://docs.qgroundcontrol.com/master/",
  ]) {
    assert.deepEqual(resolveNavigation(versionNav, new URL(href)), versionNav);
  }
});

test("nested groups inherit preservePath and individual items can opt out", () => {
  const plain = { ...versionNav[0].items[0], preservePath: false };
  const nav = [{ ...versionNav[0], items: [
    { text: "Nested", items: versionNav[0].items }, plain,
  ] }];
  const resolved = resolveNavigation(nav, new URL("https://docs.qgroundcontrol.com/master/en/page.html"));
  assert.match(resolved[0].items[0].items[0].link, /Stable_V5\.1\/en\/page\.html$/);
  assert.deepEqual(resolved[0].items[1], plain);
});

test("non-version navigation and accessibility labels remain unchanged", () => {
  const current = new URL("https://docs.qgroundcontrol.com/master/en/page.html");
  assert.deepEqual(resolveNavigation(navbar.nav, current).slice(0, -1), navbar.nav.slice(0, -1));
});

test("cached navigation is applied before fresh navigation and recached", async (t) => {
  const { saved, warnings } = prepareRuntime(t, JSON.stringify({ nav: cachedNav }));
  const applied = [];
  await refreshNavigation(remoteUrl, (nav) => applied.push(nav));
  assert.deepEqual(applied, [cachedNav, freshNav]);
  assert.deepEqual(saved, [[remoteUrl, { nav: freshNav }]]);
  assert.deepEqual(warnings, []);
});

test("failed fetch retains initial navigation and reports the error", async (t) => {
  const { fetchMock, warnings, saved } = prepareRuntime(t);
  fetchMock.mock.mockImplementation(async () => { throw new TypeError("Offline"); });
  let nav = initialNav;
  await refreshNavigation(remoteUrl, (value) => { nav = value; });
  assert.equal(nav, initialNav);
  assert.equal(warnings.length, 1);
  assert.match(warnings[0][0], /Unable to refresh/);
  assert.deepEqual(saved, []);
});

test("HTTP errors retain cached navigation", async (t) => {
  const { fetchMock, warnings } = prepareRuntime(t, JSON.stringify({ nav: cachedNav }));
  fetchMock.mock.mockImplementation(async () => new Response(null, { status: 503 }));
  let nav = initialNav;
  await refreshNavigation(remoteUrl, (value) => { nav = value; });
  assert.deepEqual(nav, cachedNav);
  assert.match(warnings[0][1].message, /HTTP 503/);
});

test("invalid remote navigation cannot replace a valid cache", async (t) => {
  const { fetchMock, saved, warnings } = prepareRuntime(t, JSON.stringify({ nav: cachedNav }));
  fetchMock.mock.mockImplementation(async () => Response.json({ nav: [{ text: "Broken" }] }));
  let nav;
  await refreshNavigation(remoteUrl, (value) => { nav = value; });
  assert.deepEqual(nav, cachedNav);
  assert.deepEqual(saved, []);
  assert.equal(warnings.length, 1);
});

test("malformed cache is reported without blocking fresh navigation", async (t) => {
  const { warnings } = prepareRuntime(t, "{");
  const applied = [];
  await refreshNavigation(remoteUrl, (nav) => applied.push(nav));
  assert.deepEqual(applied, [freshNav]);
  assert.match(warnings[0][0], /Unable to read cached/);
});

test("unavailable localStorage does not block fresh navigation", async (t) => {
  const { warnings } = prepareRuntime(t);
  Object.defineProperty(globalThis, "localStorage", {
    configurable: true,
    get() { throw new Error("Storage disabled"); },
  });
  const applied = [];
  await refreshNavigation(remoteUrl, (nav) => applied.push(nav));
  assert.deepEqual(applied, [freshNav]);
  assert.equal(warnings.length, 1);
});

test("cache write failures do not discard fresh navigation", async (t) => {
  const { storage, warnings } = prepareRuntime(t);
  storage.setItem = () => { throw new Error("Storage full"); };
  let nav;
  await refreshNavigation(remoteUrl, (value) => { nav = value; });
  assert.deepEqual(nav, freshNav);
  assert.match(warnings[0][0], /Unable to cache/);
});
