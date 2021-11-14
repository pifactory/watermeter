const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const {repInterval} = require('zigbee-herdsman-converters/lib/constants');
const extend = require('zigbee-herdsman-converters/lib/extend');
const e = exposes.presets;
const ea = exposes.access;

const fzWaterVolume = {
    cluster: 'genAnalogInput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        return {
            waterVolume: msg.data.presentValue,
            errors: msg.data.minPresentValue
        };
    },
}
const eWaterVolume = exposes.numeric('waterVolume', ea.STATE).withUnit('l').withDescription('Passed water volume since last reset')
const eErrors = exposes.numeric('errors', ea.STATE).withUnit('l').withDescription('Unreliably counted volume')
const reportingWaterVolume = async (endpoint, overrides) => {
    const p = reporting.payload('presentValue', 0, repInterval.HOUR, 0, overrides);
    await endpoint.configureReporting('genAnalogInput', p);
}
const reportingErrors = async (endpoint, overrides) => {
    const p = reporting.payload('minPresentValue', 0, repInterval.HOUR, 0, overrides);
    await endpoint.configureReporting('genAnalogInput', p);
}


module.exports = [
    {
        zigbeeModel: ['PiWaterMeter'], 
        model: 'PiWaterMeter',
        vendor: 'PiFactory',
        description: 'Smart Water Meter Sensor',
        fromZigbee: [fz.battery, fzWaterVolume],
        toZigbee: [],
        exposes: [
            e.battery(),
            e.battery_voltage(),
            eWaterVolume,
            eErrors
        ],
        configure: async (device, coordinatorEndpoint, logger) => {
            const firstEndpoint = device.getEndpoint(1);
            await reporting.bind(firstEndpoint, coordinatorEndpoint, [
                'genPowerCfg', 
                'genAnalogInput'
            ]);
            const overrides = {min: 0, max: 3600, change: 0};
            await reporting.batteryVoltage(firstEndpoint, 
                {min: 3600, max: 3600, change: 1});
            await reporting.batteryPercentageRemaining(firstEndpoint, 
                {min: 3600, max: 3600, change: 1});
            await reportingWaterVolume(firstEndpoint, 
                {min: 0, max: 3600, change: 0});
            await reportingErrors(firstEndpoint, 
                {min: 0, max: 3600, change: 0});
        },
    }
];