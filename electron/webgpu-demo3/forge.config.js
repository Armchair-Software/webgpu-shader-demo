const { FusesPlugin } = require('@electron-forge/plugin-fuses');
const { FuseV1Options, FuseVersion } = require('@electron/fuses');

module.exports = {
  packagerConfig: {
    asar: true,
  },
  rebuildConfig: {},
  makers: [
    {
      name: '@electron-forge/maker-squirrel',
      config: {
        authors: 'Armchair Software',
        description: 'Armchair Software WebGPU Demo'
        //certificateFile: './cert.pfx',
        //certificatePassword: process.env.CERTIFICATE_PASSWORD
      },
    },
    {
      name: '@electron-forge/maker-zip',
      platforms: ['linux', 'darwin', 'win32'],
    },
    {
      name: '@electron-forge/maker-deb',
      config: {
        // see https://github.com/electron/forge/blob/37a66f2a42e2ea74a30d168a32aff69c90cc20b9/packages/maker/deb/src/Config.ts#L212
        maintainer: 'Armchair Software',
        homepage: 'https://armchair.software'
      },
    },
    //{
    //  name: '@electron-forge/maker-rpm',
    //  config: {
    //    // see https://github.com/electron/forge/blob/37a66f2a42e2ea74a30d168a32aff69c90cc20b9/packages/maker/rpm/src/Config.ts#L99
    //    homepage: 'https://armchair.software'
    //  },
    //},
  ],
  plugins: [
    {
      name: '@electron-forge/plugin-webpack',
      config: {
        mainConfig: './webpack.main.config.js',
        renderer: {
          config: './webpack.renderer.config.js',
          entryPoints: [
            {
              html: './src/index.html',
              js: './src/renderer.js',
              name: 'main_window',
              preload: {
                js: './src/preload.js',
              },
            },
          ],
        },
      },
    },
    // Fuses are used to enable/disable various Electron functionality
    // at package time, before code signing the application
    //new FusesPlugin({
    //  version: FuseVersion.V1,
    //  [FuseV1Options.RunAsNode]: false,
    //  [FuseV1Options.EnableCookieEncryption]: true,
    //  [FuseV1Options.EnableNodeOptionsEnvironmentVariable]: false,
    //  [FuseV1Options.EnableNodeCliInspectArguments]: false,
    //  [FuseV1Options.EnableEmbeddedAsarIntegrityValidation]: true,
    //  [FuseV1Options.OnlyLoadAppFromAsar]: true,
    //}),
  ],
};
