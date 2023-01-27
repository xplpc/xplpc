import MainPage from "@/components/pages/MainPage.vue";
import { mount } from "@vue/test-utils";
import { createPinia, setActivePinia } from 'pinia';

describe("MainPage", () => {
    beforeEach(() => {
        setActivePinia(createPinia())
    });

    it("check if show loading message", async () => {
        const msg = "Loading...";
        const wrapper = mount(MainPage, { props: {} });
        const expected = wrapper.text();
        expect(expected).toMatch(msg);
    });
});
