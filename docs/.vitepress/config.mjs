const getSidebar = require("./get_sidebar.js");
import { defineConfig } from "vitepress";

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "QGC Guide (master)",
  description:
    "How to use and develop QGroundControl for PX4 or ArduPilot powered vehicles.",
  ignoreDeadLinks: true, // Do this for stable, where we don't yet have all translations
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

    nav: [
      {
        text: "QGroundControl",
        items: [
          {
            text: "Website",
            link: "http://qgroundcontrol.com/",
            ariaLabel: "QGC website link",
          },
          {
            text: "Download QGC (stable)",
            link: "https://docs.qgroundcontrol.com/master/en/qgc-user-guide/getting_started/download_and_install.html",
            ariaLabel: "Download stable QGC",
          },
          {
            text: "Download QGC (daily build)",
            link: "https://docs.qgroundcontrol.com/master/en/qgc-user-guide/releases/daily_builds.html",
            ariaLabel: "Download stable QGC",
          },
          {
            text: "Source Code",
            link: "https://github.com/mavlink/qgroundcontrol",
          },
          {
            text: "Docs Source Code",
            link: "https://github.com/mavlink/qgroundcontrol/tree/master/docs",
          },
        ],
      },
      {
        text: "Flight Stacks",
        items: [
          {
            text: "PX4",
            link: "https://docs.px4.io/en/",
            ariaLabel: "PX4 docs link",
          },
          {
            text: "ArduPilot",
            link: "http://ardupilot.org",
            ariaLabel: "ArduPilot site link",
          },
        ],
      },
      {
        text: "Dronecode",
        items: [
          {
            text: "PX4",
            link: "https://px4.io/",
            ariaLabel: "PX4 website link",
          },
          {
            text: "QGroundControl",
            link: "http://qgroundcontrol.com/",
          },
          {
            text: "MAVSDK",
            link: "https://mavsdk.mavlink.io/",
          },
          {
            text: "MAVLINK",
            link: "https://mavlink.io/en/",
          },
          {
            text: "Dronecode Camera Manager",
            link: "https://camera-manager.dronecode.org/en/",
          },
        ],
      },
      {
        text: "Support",
        link: "https://docs.qgroundcontrol.com/master/en/qgc-user-guide/support/support.html",
      },
      {
        text: "Version",
        items: [
          {
            text: "master",
            link: "https://docs.qgroundcontrol.com/master/en/",
          },
          {
            text: "v4.4",
            link: "https://docs.qgroundcontrol.com/Stable_V4.4/en/",
          },
        ],
      },
    ],

    socialLinks: [
      { icon: "github", link: "https://github.com/mavlink/qgroundcontrol" },
    ],
  },
});
