from django.urls import path
from . import views

urlpatterns = [
    path("", views.index, name="index"),
    path("login", views.login, name="login"),
    path("register", views.register, name="register"),
    path("logout", views.logout, name="logout"),
    path("search", views.search, name="search"),
    path("product/<int:productId>",views.product, name="product"),
    path("review/<int:productId>",views.review, name="review"),
    path("addToCart/<int:productId>",views.addToCart, name="addToCart"),
    path("checkout",views.checkout, name="checkout"),
    path("placeOrder",views.placeOrder, name="placeOrder"),
    path("viewOrders",views.viewOrders, name="viewOrders"),
    path("viewOrderDetails/<int:orderId>",views.viewOrderDetails, name="viewOrderDetails"),
    path("markOrderComplete/<int:orderId>",views.markOrderComplete, name="markOrderComplete"),
    path("addNewItem", views.addNewItem, name="addNewItem"),
    path("removeFromCart/<int:orderItemId>", views.removeFromCart, name="removeFromCart"),

]
