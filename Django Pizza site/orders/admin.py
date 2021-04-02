from django.contrib import admin

from .models import MenuItem, Category, AddOnType, AddOn, Size, OrderStatus, Order, OrderItem, AddOnCount

# Register your models here.
admin.site.register(MenuItem)
admin.site.register(Category)
admin.site.register(AddOnType)
admin.site.register(AddOn)
admin.site.register(Size)
admin.site.register(OrderStatus)
admin.site.register(Order)
admin.site.register(OrderItem)
admin.site.register(AddOnCount)
