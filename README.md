# QSingleInstance
A simple single instance application for Qt

## Features
Allows you to run an application as "single instance". This means, only one instance at a time of your application will be running. Subsequent starts will only send their arguments to this running instance, instead of starting a new one.

## Installation
The package is providet as qpm package, [`de.skycoder42.qsingleinstance`](https://www.qpm.io/packages/de.skycoder42.qsingleinstance/index.html). To install:

1. Install qpm (See [GitHub - Installing](https://github.com/Cutehacks/qpm/blob/master/README.md#installing))
2. In your projects root directory, run `qpm install de.skycoder42.qsingleinstance`
3. Include qpm to your project by adding `include(vendor/vendor.pri)` to your `.pro` file

Check their [GitHub - Usage for App Developers](https://github.com/Cutehacks/qpm/blob/master/README.md#usage-for-app-developers) to learn more about qpm.
