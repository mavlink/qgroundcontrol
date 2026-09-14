const getSidebar = require("./get_sidebar.js");
import { defineConfig } from "vitepress";
import navbarData from "./navbar.json";

const checkEnglish = process.env.QGC_DOCS_CHECK_ENGLISH === "1";

// Existing translated links are maintained separately. English CI disables all exceptions.
const translationLinkExceptions = [
  "./../../../../cmake/CustomOptions.cmake",
  "./../../../../src/AppSettings/pages",
  "./../../../../src/QmlControls/AppSettings.qml",
  "./../../../../tools/generators/config_qml/README",
  "./../../../../tools/generators/settings_qml/README",
  "./../../../../tools/generators/settings_qml/generate_pages.py",
  "./../../../../tools/generators/settings_qml/page_generator.py",
  "./getting_started/ui_overview",
  "./qgc-user-guide/getting_started/ui_overview"
];

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: process.env.BRANCH_NAME
    ? `QGC Guide (${process.env.BRANCH_NAME})`
    : "QGC Guide",
  description:
    "How to use and develop QGroundControl for PX4 or ArduPilot powered vehicles.",
  ignoreDeadLinks: checkEnglish ? false : translationLinkExceptions,
  srcExclude: checkEnglish ? ["ko/**", "tr/**", "zh/**"] : [],
  base: process.env.BRANCH_NAME ? "/" + process.env.BRANCH_NAME + "/" : "",

  head: [
    [
      "script",
      {
        async: "",
        src: "https://www.googletagmanager.com/gtag/js?id=UA-33658859-3",
      },
    ],
    [
      "script",
      {},
      `window.dataLayer = window.dataLayer || [];
      function gtag(){dataLayer.push(arguments);}
      gtag('js', new Date());
      gtag('config', 'UA-33658859-3');`,
    ],
  ],

  locales: {
    en: {
      label: "English",
      //lang: "en",
      themeConfig: {
        sidebar: getSidebar.sidebar({ lang: "en" }),

        editLink: {
          pattern:
            "https://github.com/mavlink/qgroundcontrol/edit/master/docs/:path",
          text: "Edit on GitHub",
        },
      },
    },
    zh: {
      label: "中文 (Chinese)",
      lang: "zh-CN", // optional, will be added  as `lang` attribute on `html` tag
      themeConfig: {
        sidebar: getSidebar.sidebar({ lang: "zh" }),
      },
      // other locale specific properties...
    },
    ko: {
      label: "한국어 (Korean)",
      lang: "ko-KR", // optional, will be added  as `lang` attribute on `html` tag
      themeConfig: {
        sidebar: getSidebar.sidebar({ lang: "ko" }),
      },

      // other locale specific properties...
    },
    tr: {
      label: "Türkçe (Turkish)",
      lang: "tr-TR", // optional, will be added  as `lang` attribute on `html` tag
      themeConfig: {
        sidebar: getSidebar.sidebar({ lang: "tr" }),
      },

      // other locale specific properties...
    },
  },

  themeConfig: {
    // https://vitepress.dev/reference/default-theme-config
    logo: "qgc_icon.png",
    //sidebar: getSidebar.sidebar({ lang: "en" }),
    search: {
      provider: "local",
    },

    dynamicNavUrl:
      "https://raw.githubusercontent.com/mavlink/qgroundcontrol/master/docs/.vitepress/navbar.json",
    nav: navbarData.nav,

    socialLinks: [
      { icon: "github", link: "https://github.com/mavlink/qgroundcontrol" },
    ],
  },
});
