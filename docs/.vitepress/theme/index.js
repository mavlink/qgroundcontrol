// https://vitepress.dev/guide/custom-theme
import { h } from "vue";
import DefaultTheme from "vitepress/theme";
import "./style.css";

// Toast shown once per page load, announcing the current docs version
import VersionToast from "./components/VersionToast.vue";

import { createDynamicNav, DynamicNav } from "vp-dynamic-nav";

/** @type {import('vitepress').Theme} */
export default {
  // createDynamicNav(DefaultTheme).Layout is a parameterless function that
  // doesn't forward slots passed to it, so its nav-bar slots are reproduced
  // here directly against DefaultTheme.Layout instead of nesting through it
  // (nesting silently drops any slot we'd add, e.g. layout-top for the toast).
  extends: createDynamicNav(DefaultTheme),
  Layout: () =>
    h(DefaultTheme.Layout, null, {
      "nav-bar-content-before": () => h(DynamicNav),
      "nav-screen-content-after": () => h(DynamicNav, { screen: true }),
      "layout-top": () => h(VersionToast),
    }),
  enhanceApp({ app, router, siteData }) {
    // ...
  },
};
