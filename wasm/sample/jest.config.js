module.exports = {
    moduleFileExtensions: ["js", "ts", "json", "vue"],
    testEnvironment: "jsdom",
    transform: {
        "^.+\\.ts$": "ts-jest",
        "^.+\\.vue$": "@vue/vue3-jest",
    },
    preset: '@vue/cli-plugin-unit-jest'
};
