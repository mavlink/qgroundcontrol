function isNavigationItem(item) {
  if (!item || typeof item !== "object" || typeof item.text !== "string") {
    return false;
  }
  if (item.preservePath !== undefined && typeof item.preservePath !== "boolean") {
    return false;
  }
  if (item.items !== undefined) {
    return Array.isArray(item.items) && item.items.length > 0 &&
      item.link === undefined && item.items.every(isNavigationItem);
  }
  if (typeof item.link !== "string" || !item.link.trim()) {
    return false;
  }
  try {
    const url = new URL(item.link, "https://docs.qgroundcontrol.com/");
    return url.protocol === "https:" || url.protocol === "http:";
  } catch {
    return false;
  }
}

export function parseNavigation(data) {
  if (!Array.isArray(data?.nav) || !data.nav.length || !data.nav.every(isNavigationItem)) {
    throw new TypeError("Invalid docs navigation data");
  }
  return data.nav;
}

export function resolveNavigation(nav, currentUrl, inheritedPreservePath = false) {
  return nav.map((item) => {
    const preservePath = item.preservePath ?? inheritedPreservePath;
    if (item.items) {
      return {
        ...item,
        items: resolveNavigation(item.items, currentUrl, preservePath),
      };
    }
    if (!preservePath || !currentUrl) {
      return item;
    }

    const target = new URL(item.link, currentUrl);
    const targetSegments = target.pathname.split("/").filter(Boolean);
    const currentSegments = currentUrl.pathname.split("/").filter(Boolean);
    if (target.origin !== currentUrl.origin || currentSegments.length < targetSegments.length) {
      return item;
    }

    const suffix = currentSegments.slice(targetSegments.length);
    const pathname = "/" + [...targetSegments, ...suffix].join("/");
    const keepTrailingSlash = suffix.length ? currentUrl.pathname.endsWith("/") : target.pathname.endsWith("/");
    const trailingSlash = keepTrailingSlash && pathname !== "/" ? "/" : "";
    return { ...item, link: target.origin + pathname + trailingSlash + currentUrl.hash, target: "_self" };
  });
}

export async function refreshNavigation(remoteUrl, applyNavigation) {
  let storage;
  let cachedNav;
  try {
    storage = globalThis.localStorage;
    const cached = storage.getItem(remoteUrl);
    if (cached) {
      cachedNav = parseNavigation(JSON.parse(cached));
    }
  } catch (error) {
    console.warn("Unable to read cached docs navigation:", error);
  }
  if (cachedNav) {
    applyNavigation(cachedNav);
  }

  let nav;
  try {
    const response = await fetch(remoteUrl);
    if (!response.ok) {
      throw new Error(`Docs navigation request failed: HTTP ${response.status}`);
    }
    nav = parseNavigation(await response.json());
  } catch (error) {
    console.warn("Unable to refresh docs navigation; keeping existing navigation:", error);
    return;
  }
  applyNavigation(nav);

  try {
    storage?.setItem(remoteUrl, JSON.stringify({ nav }));
  } catch (error) {
    console.warn("Unable to cache docs navigation:", error);
  }
}
