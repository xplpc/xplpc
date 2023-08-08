import asyncio
import logging

import psutil
from kivy.app import async_runTouchApp
from kivy.core.window import Window
from kivy.uix.scrollview import ScrollView
from kivymd.app import MDApp
from kivymd.uix.boxlayout import MDBoxLayout
from kivymd.uix.button import MDFlatButton
from kivymd.uix.label import MDLabel
from kivymd.uix.selectioncontrol import MDCheckbox
from kivymd.uix.textfield import MDTextField

from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.map.mapping_item import MappingItem
from xplpc.message.message import Message
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.message.response import Response
from xplpc.serializer.json_serializer import JsonSerializer

logging.basicConfig(level=logging.DEBUG)

serializer = JsonSerializer()
config = Config(serializer)
XPLPC().initialize(config)


global_loop = asyncio.get_event_loop()


def battery_level(m: Message, r: Response):
    async def main():
        # async sleep
        await asyncio.sleep(0.1)

        # return response
        suffix = m.get("suffix")
        battery = psutil.sensors_battery()
        r(f"{battery.percent}{suffix}")

    global_loop.create_task(main())


MappingList().add(
    "platform.battery.level",
    MappingItem(battery_level),
)


class PaddedScrollView(ScrollView):
    def __init__(self, **kwargs):
        super(PaddedScrollView, self).__init__(**kwargs)
        self.padding = ("20dp", "20dp", "20dp", "20dp")


class MyApp(MDApp):
    def build(self):
        # general
        scroll_view = PaddedScrollView(size_hint=(1, 1), do_scroll_x=False)
        main_box = MDBoxLayout(orientation="vertical", spacing="10dp", padding="10dp")

        # battery level
        battery_label = MDLabel(
            text="BATTERY LEVEL", font_style="H5", halign="center", bold=True
        )
        main_box.add_widget(battery_label)

        self.battery_response = MDLabel(
            text="Press Button To Execute",
            halign="center",
            theme_text_color="Secondary",
        )
        main_box.add_widget(self.battery_response)

        battery_button_box = MDBoxLayout(
            orientation="horizontal",
            size_hint=(1, None),
            height="48dp",
            padding="10dp",
            spacing="10dp",
        )

        self.battery_button = MDFlatButton(
            text="SUBMIT",
            halign="center",
            size_hint_x=None,
            theme_text_color="Custom",
            text_color=(1, 1, 1, 1),
            md_bg_color=(0, 0.447, 0.741, 1),
            padding=(50, 0, 50, 0),
        )

        self.battery_button.bind(on_release=self.on_battery_button_clicked)
        battery_button_box.add_widget(MDBoxLayout(size_hint=(1, None)))
        battery_button_box.add_widget(self.battery_button)
        battery_button_box.add_widget(MDBoxLayout(size_hint=(1, None)))
        main_box.add_widget(battery_button_box)

        # login form
        login_label = MDLabel(text="LOGIN", font_style="H5", halign="center", bold=True)
        main_box.add_widget(login_label)

        self.username_text = MDTextField(
            hint_text="Your username",
            multiline=False,
            halign="center",
            size_hint_x=0.5,
            pos_hint={"center_x": 0.5},
        )

        main_box.add_widget(self.username_text)

        self.password_text = MDTextField(
            password=True,
            hint_text="Your password",
            multiline=False,
            halign="center",
            size_hint_x=0.5,
            pos_hint={"center_x": 0.5},
        )

        main_box.add_widget(self.password_text)

        checkbox_layout = MDBoxLayout(
            orientation="vertical",
            size_hint=(0.5, None),
            height="48dp",
            padding="10dp",
            spacing="10dp",
            pos_hint={"center_x": 0.5},
        )

        self.remember_me = MDCheckbox(
            size_hint=(None, None),
            size=("48dp", "48dp"),
            pos_hint={"center_x": 0.5},
        )
        checkbox_layout.add_widget(self.remember_me)

        self.remember_me_label = MDLabel(
            text="Remember me", halign="center", valign="middle"
        )

        checkbox_layout.add_widget(self.remember_me_label)
        main_box.add_widget(checkbox_layout)

        self.login_response = MDLabel(
            text="Press Button To Execute",
            halign="center",
            theme_text_color="Secondary",
        )
        main_box.add_widget(self.login_response)

        login_button_box = MDBoxLayout(
            orientation="horizontal",
            size_hint=(1, None),
            height="48dp",
            padding="10dp",
            spacing="10dp",
        )

        self.login_button = MDFlatButton(
            text="SUBMIT",
            halign="center",
            size_hint_x=None,
            theme_text_color="Custom",
            text_color=(1, 1, 1, 1),
            md_bg_color=(0, 0.447, 0.741, 1),
            padding=(50, 0, 50, 0),
        )

        self.login_button.bind(on_release=self.on_login_button_clicked)
        login_button_box.add_widget(MDBoxLayout(size_hint=(1, None)))
        login_button_box.add_widget(self.login_button)
        login_button_box.add_widget(MDBoxLayout(size_hint=(1, None)))

        # main container
        main_box.add_widget(login_button_box)
        scroll_view.add_widget(main_box)

        return scroll_view

    def on_battery_button_clicked(self, instance):
        asyncio.create_task(self.do_battery_click_action())

    async def do_battery_click_action(self):
        request = Request(
            "platform.battery.level",
            [
                Param("suffix", "%"),
            ],
        )
        response = await Client.async_call(request)
        self.battery_response.text = f"Response: {response}"

    def on_login_button_clicked(self, instance):
        asyncio.create_task(self.do_login_click_action())

    async def do_login_click_action(self):
        username = self.username_text.text
        password = self.password_text.text
        remember = self.remember_me.active

        request = Request(
            "sample.login",
            [
                Param("username", username),
                Param("password", password),
                Param("remember", remember),
            ],
        )
        response = await Client.async_call(request)
        self.login_response.text = f"Response: {response}"


if __name__ == "__main__":
    global_loop.run_until_complete(
        async_runTouchApp(MyApp().build(), async_lib="asyncio")
    )
