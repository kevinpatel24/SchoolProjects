from django.urls import path

from . import views

urlpatterns = [
    path("", views.index, name="index"),
    path("login", views.login, name="login"),
    path("register", views.register, name="register"),
    path("logout", views.logout, name="logout"),
    path("customize/<int:menuItem_id>",views.customize, name="customize"),
    path("addToCart/<int:menuItem_id>", views.addToCart, name="addToCart"),
    path("checkout", views.checkout, name="checkout"),
    path("placeOrder", views.placeOrder, name="placeOrder"),
    path("viewOrders", views.viewOrders, name="viewOrders"),
    path("viewOrderDetails/<int:order_id>", views.viewOrderDetails, name="viewOrderDetails"),
    path("markOrderComplete/<int:order_id>", views.markOrderComplete, name="markOrderComplete")
]
