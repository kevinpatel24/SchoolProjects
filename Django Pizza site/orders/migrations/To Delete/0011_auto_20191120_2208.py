# Generated by Django 2.2.7 on 2019-11-20 22:08

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0010_auto_20191120_2206'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='addoncount',
            name='addOnType',
        ),
        migrations.RemoveField(
            model_name='addoncount',
            name='category',
        ),
        migrations.RemoveField(
            model_name='addoncount',
            name='size',
        ),
    ]
