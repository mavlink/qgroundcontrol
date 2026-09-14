// https://vitepress.dev/guide/custom-theme
import { computed, h, onMounted, provide, ref } from "vue";
import { dataSymbol, useData, useRoute } from "vitepress";
import DefaultTheme from "vitepress/theme";
import { refreshNavigation, resolveNavigation } from "./dynamic-nav.mjs";
import "./style.css";

import VersionToast from "./components/VersionToast.vue";

/** @type {import('vitepress').Theme} */
export default {
  extends: DefaultTheme,
  setup() {
    const data = useData();
    const route = useRoute();
    const nav = ref(data.theme.value.nav);
    const mounted = ref(false);

    provide(dataSymbol, {
      ...data,
      theme: computed(() => ({
        ...data.theme.value,
        nav: resolveNavigation(
          nav.value,
          mounted.value ? new URL(route.path + route.hash, location.href) : null,
        ),
      })),
    });

    onMounted(() => {
      // Defer browser-specific links until after hydration.
      mounted.value = true;
      void refreshNavigation(data.theme.value.dynamicNavUrl, (items) => {
        nav.value = items;
      });
    });
  },
  Layout: () =>
    h(DefaultTheme.Layout, null, {
      "layout-top": () => h(VersionToast),
    }),
};
